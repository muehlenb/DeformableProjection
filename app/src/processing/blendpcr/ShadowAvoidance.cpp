#pragma once

#include "src/processing/blendpcr/ShadowAvoidance.h"

#include "src/simulation/scene/components/Projector.h"


void ShadowAvoidance::glTick(){
    CameraPasses& pctextures = CameraPasses::getInstance();

	glDisable(GL_BLEND);

	// If opengl resources are not initialized yet, do it:
    init();

	// Check if cameras for rendering are available:
	if(pctextures.usedCameraIDs.size() == 0){
		//std::cout << "ExperimentalPCPreprocessor: NO CAMERAS FOR RENDERING AVAILABLE!" << std::endl;
		return;
    }

	glDisable(GL_CULL_FACE);
	glCullFace(GL_BACK);

    // Backup viewport and framebuffer:
    int mainViewport[4];
    glGetIntegerv(GL_VIEWPORT, mainViewport);
	int originalFramebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &originalFramebuffer);

    Mat4f projectorProjectionMatrix = Mat4f::perspectiveTransformation(16.f / 9.f, 40.f, 0.01f, 1000.f);

    {
        // PASS 1:
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        for(int projectorID = 0; projectorID < Data::instance.projectors.size(); ++projectorID){
            glViewport(0, 0, fbo_screen_width, fbo_screen_height);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo_projectorDistance[projectorID]);
			std::shared_ptr<Projector> projector = Data::instance.projectors[projectorID];

            SceneData sceneData(SD_NONE);
            sceneData.view = projector->transformation.inverse();
            sceneData.projection = projectorProjectionMatrix;//projector->projectionMatrix;
            blendPCRForFirstPass->render(sceneData, Mat4f(), false, 0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // PASS 2:
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);
            glBindFramebuffer(GL_FRAMEBUFFER, pctextures.fbo_vertexShadowMap[cameraID]);
            glClear(GL_COLOR_BUFFER_BIT);

            vertexShadowMapShader.bind();
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
            vertexShadowMapShader.setUniform("vertexTexture", 1);

            vertexShadowMapShader.setUniform("model", pctextures.pointCloudMatrix[cameraID]);

            for(int projectorID = 0; projectorID < Data::instance.projectors.size(); ++projectorID){
                glActiveTexture(GL_TEXTURE2 + projectorID);
                glBindTexture(GL_TEXTURE_2D, texture2D_projectorDistanceMap[projectorID]);
                vertexShadowMapShader.setUniform("projectors["+std::to_string(projectorID)+"].distanceMap", 2 + projectorID);

                vertexShadowMapShader.setUniform("projectors["+std::to_string(projectorID)+"].projection", projectorProjectionMatrix);//Data::instance.projectors[projectorID]->projectionMatrix);
                vertexShadowMapShader.setUniform("projectors["+std::to_string(projectorID)+"].view", Data::instance.projectors[projectorID]->transformation.inverse());
            }

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // PASS 3:
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, 512, 512);
            // Jump Flooding
            {
                int max = 8;
                for(int i=0; i < max; ++i){
                    glBindFramebuffer(GL_FRAMEBUFFER, i % 2 == 0 ? pctextures.fbo_jumpFloodingPing : pctextures.fbo_jumpFloodingPong);
                    glClear(GL_COLOR_BUFFER_BIT);

                    jumpFloodingShader.bind();
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_vertexShadowMap[cameraID]);
                    jumpFloodingShader.setUniform("inputTexture", 1);

                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, i % 2 == 0 ? pctextures.texture2D_jumpFloodingPong : pctextures.texture2D_jumpFloodingPing);
                    jumpFloodingShader.setUniform("inputPingPong", 2);

                    jumpFloodingShader.setUniform("model", pctextures.pointCloudMatrix[cameraID]);

                    for(int projectorID = 0; projectorID < Data::instance.projectors.size(); ++projectorID){
                        jumpFloodingShader.setUniform("projectors["+std::to_string(projectorID)+"].projection", Data::instance.projectors[projectorID]->projectionMatrix);
                        jumpFloodingShader.setUniform("projectors["+std::to_string(projectorID)+"].view", Data::instance.projectors[projectorID]->transformation.inverse());
                    }

                    jumpFloodingShader.setUniform("k", 1 << (max-1-i));

                    jumpFloodingShader.setUniform("isFirstPass", i==0);

                    glBindVertexArray(pctextures.VAO_quad);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);
            //glViewport(0, 0, 256, 256);
            {
                glBindFramebuffer(GL_FRAMEBUFFER, pctextures.fbo_vertexDistanceMap[cameraID]);
                glClear(GL_COLOR_BUFFER_BIT);

                vertexDistanceMapShader.bind();
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
                vertexDistanceMapShader.setUniform("vertexTexture", 1);

                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_jumpFloodingPong);
                vertexDistanceMapShader.setUniform("nearestIndicesTexture", 2);

                //TODO: CHECKEN, wie die Projector Assignment berechnet wird und checken, dass vmtl. der
                //"Im Frustum" check in den distanceMap shader eingebaut werden muss. Prüfen, ob die Distanz
                //entlang des Beamer-Bildes irgendwie mit eingebaut werden kann (vmtl. aber nicht möglich, da Distanz im 2D Bild... nachdenken!)

                vertexDistanceMapShader.setUniform("model", pctextures.pointCloudMatrix[cameraID]);

                for(int projectorID = 0; projectorID < Data::instance.projectors.size(); ++projectorID){
                    /** Projectors viewing direction in World Space: */
                    vertexDistanceMapShader.setUniform("projectors["+std::to_string(projectorID)+"].direction", Data::instance.projectors[projectorID]->transformation * Vec4f(0, 0, 1, 0));
                }

                glBindVertexArray(pctextures.VAO_quad);
                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            //if(cameraID == 0){
            //    Data::instance.texture_debugSlot2 = texture2D_vertexDistanceMap[cameraID];
            //}
        }

        // PASS 4 (Global Distance Map):
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);
            glBindFramebuffer(GL_FRAMEBUFFER, pctextures.fbo_globalDistanceMap[cameraID]);
            glClear(GL_COLOR_BUFFER_BIT);

            globalDistanceMapShader.bind();

            int textureSlot = 1;
            for(unsigned int cID : pctextures.usedCameraIDs){
                glActiveTexture(GL_TEXTURE0 + textureSlot);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cID]);
                globalDistanceMapShader.setUniform("vertexTextures["+std::to_string(cID)+"]", textureSlot);
                ++textureSlot;

                glActiveTexture(GL_TEXTURE0 + textureSlot);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_vertexDistanceMap[cID]);
                globalDistanceMapShader.setUniform("distanceTextures["+std::to_string(cID)+"]", textureSlot);
                ++textureSlot;

                glActiveTexture(GL_TEXTURE0 + textureSlot);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_inputLookupImageTo3D[cID]);
                globalDistanceMapShader.setUniform("lookupImageTo3D["+std::to_string(cID)+"]", textureSlot);
                ++textureSlot;

                glActiveTexture(GL_TEXTURE0 + textureSlot);
                glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_inputLookup3DToImage[cID]);
                globalDistanceMapShader.setUniform("lookup3DToImage["+std::to_string(cID)+"]", textureSlot);
                ++textureSlot;

                globalDistanceMapShader.setUniform("model["+std::to_string(cID)+"]", pctextures.pointCloudMatrix[cID]);
                globalDistanceMapShader.setUniform("view["+std::to_string(cID)+"]", pctextures.pointCloudMatrix[cID].inverse());
            }

            globalDistanceMapShader.setUniform("cameraID", int(cameraID));
            globalDistanceMapShader.setUniform("cameraNum", int(pctextures.usedCameraIDs.size()));

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if(cameraID == 0){
                //Data::instance.texture_debugSlot2 = texture2D_globalDistanceMap[cameraID];
            }
        }

        // Pass 5 (Temporal distance map):
        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);

            unsigned int currentFBO = pctextures.temporalDistanceFlipFlop ? pctextures.fbo_temporalDistanceMapB[cameraID] : pctextures.fbo_temporalDistanceMapA[cameraID];
            unsigned int previousTexture = pctextures.temporalDistanceFlipFlop ? pctextures.texture2D_temporalDistanceMapA[cameraID] : pctextures.texture2D_temporalDistanceMapB[cameraID];

            glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);
            glClear(GL_COLOR_BUFFER_BIT);

            temporalDistanceMapShader.bind();
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_globalDistanceMap[cameraID]);
            temporalDistanceMapShader.setUniform("currentDistanceMap", 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, previousTexture);
            temporalDistanceMapShader.setUniform("previousDistanceMap", 2);

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        pctextures.temporalDistanceFlipFlop = !pctextures.temporalDistanceFlipFlop;

        // Segmentation Downscale:
        GLenum attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        //glViewport(0, 0, 256, 256);
        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);

            glBindFramebuffer(GL_FRAMEBUFFER, pctextures.fbo_segmentationDownscale[cameraID]);
            glDrawBuffers(2, attachments);
            glClear(GL_COLOR_BUFFER_BIT);

            segmentationDownscaleShader.bind();
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
            segmentationDownscaleShader.setUniform("vertexTexture", 1);

            if(uiRenderer != nullptr){
                uiRenderer->bindShadowTileTexture(2);
            	segmentationDownscaleShader.setUniform("segmentationTexture", 2);
			}

            segmentationDownscaleShader.setUniform("camModel", pctextures.pointCloudMatrix[cameraID]);
            segmentationDownscaleShader.setUniform("inverseUIMatrix", Data::instance.virtualDisplayTransform.inverse());
            segmentationDownscaleShader.setUniform("spectatorPosWS", Data::instance.virtualDisplaySpectator);

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if(cameraID == 0){
                //Data::instance.texture_debugSlot2 = texture2D_segmentationShadowDistance[cameraID];
            }
        }

        // Pass 6 (Projector Assignment):
        for(unsigned int cameraID : pctextures.usedCameraIDs){
            if(!pctextures.cameraIsUpdatedThisFrame[cameraID])
                continue;

            glViewport(0, 0, pctextures.cameraWidth[cameraID], pctextures.cameraHeight[cameraID]);

            glBindFramebuffer(GL_FRAMEBUFFER, pctextures.fbo_vertexProjectorAssignment[cameraID]);
            glDrawBuffers(1, attachments);
            glClear(GL_COLOR_BUFFER_BIT);

            vertexProjectorAssignmentShader.bind();
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, pctextures.texture2D_mlsVertices[cameraID]);
            vertexProjectorAssignmentShader.setUniform("vertexTexture", 1);

            glActiveTexture(GL_TEXTURE2);

            //glBindTexture(GL_TEXTURE_2D, texture2D_globalDistanceMap[cameraID]);

            glBindTexture(GL_TEXTURE_2D, pctextures.temporalDistanceFlipFlop ? pctextures.texture2D_temporalDistanceMapA[cameraID] : pctextures.texture2D_temporalDistanceMapB[cameraID]);

            vertexProjectorAssignmentShader.setUniform("distanceTexture", 2);

            vertexProjectorAssignmentShader.setUniform("model", pctextures.pointCloudMatrix[cameraID]);

            for(int projectorID = 0; projectorID < Data::instance.projectors.size(); ++projectorID){
                /** Projectors viewing direction in World Space: */
                vertexProjectorAssignmentShader.setUniform("projectors["+std::to_string(projectorID)+"].position", Data::instance.projectors[projectorID]->transformation.getPosition());
                vertexProjectorAssignmentShader.setUniform("projectors["+std::to_string(projectorID)+"].direction", Data::instance.projectors[projectorID]->transformation * Vec4f(0, 0, 1, 0));

                vertexProjectorAssignmentShader.setUniform("projectors["+std::to_string(projectorID)+"].projection", Data::instance.projectors[projectorID]->projectionMatrix);
                vertexProjectorAssignmentShader.setUniform("projectors["+std::to_string(projectorID)+"].view", Data::instance.projectors[projectorID]->transformation.inverse());
            }

            glBindVertexArray(pctextures.VAO_quad);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            if(cameraID == 0){
                //Data::instance.texture_debugSlot3 = texture2D_vertexProjectorAssignment[cameraID];
            }
        }
	}

    // Restore viewport and framebuffer:
    glViewport(mainViewport[0], mainViewport[1], mainViewport[2], mainViewport[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, originalFramebuffer);

    glFlush();
}
