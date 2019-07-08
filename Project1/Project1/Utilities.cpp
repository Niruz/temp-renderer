#include <iostream>

//why is fopen unsafe :(
#pragma warning(disable:4996)

#pragma pack(push, 1) //Courtesy of Casey 
struct bitmap_header 
{
	unsigned short FileType;
	unsigned int FileSize;
	unsigned short Reserved1;
	unsigned short Reserved2;
	unsigned int BitmapOffset;
	unsigned int Size;
	int Width;
	int Height;
	unsigned short Planes;
	unsigned short BitsPerPixel;
};
#pragma pack(pop)

static unsigned char* 
LoadBMP(const char* filename, int& width, int& height)
{
	FILE* f = fopen(filename, "rb");

	if (f == NULL)
		std::cout << "File: " << filename << " not found." << std::endl;

	unsigned char info[54];
	fread(info, sizeof(unsigned char), 54, f); // read the 54-byte header

											
	bitmap_header *Header = (bitmap_header*)info; 

	std::cout << "Name: " << filename << std::endl;
	std::cout << "Width: " << Header->Width << std::endl;
	std::cout << "Height: " << Header->Height << std::endl;
	std::cout << "Planes: " << Header->Planes << std::endl;
	std::cout << "Bits: " << Header->BitsPerPixel << std::endl;

	width = Header->Width;
	height = Header->Height;

	unsigned int size = 4 * width * height;
	unsigned char* data = new unsigned char[size]; // allocate 4 bytes per pixel
	fseek(f, Header->BitmapOffset, SEEK_SET);
	fread(data, sizeof(unsigned char), size, f);


	fclose(f);

	return data;
}