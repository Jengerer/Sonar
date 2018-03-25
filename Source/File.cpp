#include "File.h"
#include <fstream>

using namespace Sonar;
using namespace std;

U8* Sonar::Read(const Char* filename)
{
	ifstream inputStream(filename, ios::binary | ios::ate);
	const ifstream::pos_type endPosition = inputStream.tellg();
	const Size fileSize = static_cast<Size>(endPosition);
	
	// Read bytes in
	static_assert(sizeof(Char) == sizeof(U8), "Mismatch in buffer granularity!");
	Char* buffer = new Char[fileSize];
	inputStream.seekg(0, ios::beg);
	inputStream.read(buffer, fileSize);
	inputStream.close();
	U8* result = reinterpret_cast<U8*>(buffer);
	return(result);
}
