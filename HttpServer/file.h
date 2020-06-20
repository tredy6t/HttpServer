#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>


std::string ReadFile(const std::string& strFileName)
{
	std::string strFileData;
	std::fstream fin(strFileName.c_str(), std::ios::in);
	if (fin.is_open())
	{
		fin.seekg(0, std::ios::end);
		std::streampos size = fin.tellp();
		fin.seekg(0, std::ios::beg);
		std::vector<char>vecData(size);
		fin.read(vecData.data(), size);
		fin.close();
		strFileData.assign((char*)vecData.data(), size);
	}
	return strFileData;
}

std::string LoadImg(const std::string& strPic)
{
	std::string strFileData;
	std::fstream fin(strPic.c_str(), std::ios::in | std::ios::binary);
	if (fin.is_open())
	{
		fin.seekg(0, std::ios::end);
		std::streampos size = fin.tellp();
		fin.seekg(0, std::ios::beg);
		std::vector<char>vecData(size);
		fin.read(vecData.data(), size);
		fin.close();
		strFileData.assign((char*)vecData.data(), size);
	}
	return strFileData;
}

void SavePic(const std::string& strPic, const std::string& strPicData)
{
	std::fstream fout(strPic.c_str(), std::ios::out | std::ios::binary);
	if (fout.is_open())
	{
		fout.write(strPicData.c_str(), strPicData.size());
		fout.close();
	}
}
