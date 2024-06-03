#pragma once
#ifndef GVREADER_H
#define GVREADER_H
#include "BaseHeader.h"

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

//Reads a vector from a string in the manner of: 40,50,-20
vector3df strToVec(std::string str);

/*
* Reads in data from a .gdat file (or really any text file, it's not picky). Includes functions
* to map the lines read in to a map of values (such as driver=Direct3D9), and vice versa. Will write out
* to a given file.
*/
struct gvReader
{
	//Lines of a loaded file.
	std::vector<std::string> lines;
	//Map of values.
	std::map<std::string, std::string> values;

	//Reads the lines currently loaded to the map.
	void readLinesToValues();
	//Reads the map back out to lines of text.
	void readValuesToLines();
	//Reads in lines from a file.
	void read(std::string filename);
	//Writes lines to a file.
	void write(std::string filename);

	//Checks if the file read has this key in it. Always returns fales if lines have not been read to values.
	bool hasVal(std::string key);
	//Casts the appropriate value to an int and returns it.
	int getInt(std::string key);
	//Casts the value to an unsigned int and returns it.
	u32 getUint(std::string key);
	//Casts the appropriate value to a float and returns it.
	f32 getFloat(std::string key);
	//Casts the appropriate value into a 3D vector and returns it.
	vector3df getVec(std::string key);
	//Casts the appropriate value to a bool and returns it.
	bool getBool(std::string key);
	//Returns the appropriate value.
	std::string getString(std::string key);
	//Casts the appropriate value to an SColorf and returns it. Uses RGBA.
	SColorf getColorf(std::string key);
	//Casts the appropriate value to an SColor and returns it. Uses RGBA.
	SColor getColor(std::string key);
	//Casts the appropriate value to an SColor and returns it. Uses irrlicht ARGB.
	SColor getColorSilly(std::string key);
	void clear();
};

#endif 