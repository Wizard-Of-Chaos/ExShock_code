#include "GvReader.h"

vector3df strToVec(std::string str) //Turns a string to a vector, splitting on a , character
{
	std::string xstr, ystr, zstr;

	std::stringstream split(str);
	std::getline(split, xstr, ',');
	std::getline(split, ystr, ',');
	std::getline(split, zstr, ',');
	return vector3df(std::stof(xstr), std::stof(ystr), std::stof(zstr));
}

void gvReader::read(std::string filename)
{
	std::ifstream in;
	in.open(filename);
	std::string input;
	while (std::getline(in, input)) {
		lines.push_back(std::string(input));
	}
	in.close();
}

void gvReader::write(std::string filename)
{
	std::ofstream out;
	out.open(filename);
	for (std::string str : lines) {
		std::string outstr = str + '\n';
		out << outstr;
	}
	out.close();
}

void gvReader::readLinesToValues()
{
	if (lines.empty()) return;
	for (std::string str : lines) {
		std::string key;
		std::string value;
		std::stringstream split(str);
		std::getline(split, key, '=');
		std::getline(split, value, '=');
		values[key] = value;
	}
}
void gvReader::readValuesToLines()
{
	lines.clear();
	for (const auto p : values) {
		std::string out = p.first + "=" + p.second;
		lines.push_back(out);
	}
}

void gvReader::clear()
{
	lines.clear();
	values.clear();
}

bool gvReader::hasVal(std::string key)
{
	return (values.find(key) != values.end());
}
int gvReader::getInt(std::string key)
{
	return std::stoi(values[key]);
}
u32 gvReader::getUint(std::string key)
{
	return (u32)std::stoi(values[key]);
}
bool gvReader::getBool(std::string key)
{
	return (bool)std::stoi(values[key]);
}
f32 gvReader::getFloat(std::string key)
{
	return std::stof(values[key]);
}

std::string gvReader::getString(std::string key)
{
	return values[key];
}

vector3df gvReader::getVec(std::string key)
{
	return strToVec(values[key]);
}

SColorf gvReader::getColorf(std::string key)
{
	std::string r, g, b, a;

	std::stringstream split(values[key]);
	std::getline(split, r, ',');
	std::getline(split, g, ',');
	std::getline(split, b, ',');
	std::getline(split, a, ',');

	return SColorf(std::stof(r), std::stof(g), std::stof(b), std::stof(a));

}

SColor gvReader::getColor(std::string key)
{
	std::string r, g, b, a;

	std::stringstream split(values[key]);
	std::getline(split, r, ',');
	std::getline(split, g, ',');
	std::getline(split, b, ',');
	std::getline(split, a, ',');

	return SColor(std::stoi(a), std::stoi(r), std::stoi(g), std::stoi(b)); //dammit, irrlicht

}

SColor gvReader::getColorSilly(std::string key)
{
	std::string r, g, b, a;

	std::stringstream split(values[key]);
	std::getline(split, a, ',');
	std::getline(split, r, ',');
	std::getline(split, g, ',');
	std::getline(split, b, ',');

	return SColor(std::stoi(a), std::stoi(r), std::stoi(g), std::stoi(b));

}