#pragma once

#include <memory>

template<typename T>
struct Image2D {
	int width;
	int height;

	T* values;

	Image2D(int width, int height)
		: width(width)
		, height(height){
		values = new T[width * height];
		std::memset(values, 0, sizeof(T) * width * height);
	}

	// Copy-Konstruktor (tiefe Kopie)
	Image2D(const Image2D& other)
		: width(other.width), height(other.height), values(nullptr) {
		values = new T[width * height];
		std::memcpy(values, other.values, sizeof(T) * width * height);
	}

	// Assignment Operator (tiefe Kopie)
	Image2D& operator=(const Image2D& other) {
		if (this != &other) {
			// alten Speicher löschen
			delete[] values;

			// neue Größe kopieren
			width = other.width;
			height = other.height;

			// neuen Speicher allozieren und Werte kopieren
			values = new T[width * height];
			std::memcpy(values, other.values, sizeof(T) * width * height);
		}
		return *this;
	}

	~Image2D(){
		delete[] values;
		values = nullptr;
	}

	inline T get(int x, int y){
		return values[x + y * width];
	}

	inline void set(int x, int y, T value){
		values[x + width * y] = value;
	}

	inline int getWidth(){
		return width;
	}

	inline int getHeight(){
		return height;
	}
};
