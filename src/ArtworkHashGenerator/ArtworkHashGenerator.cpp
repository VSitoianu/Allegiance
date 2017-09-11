// BT - STEAM

#include "stdafx.h"

//#include "Test.cpp"

TVector<ZString, DefaultEquals, DefaultCompare> GetWhiteList()
{
	TVector<ZString, DefaultEquals, DefaultCompare> returnValue;

	// File names should always be lowercase.
	returnValue.PushEnd(ZString("dialog.mdl"));
	returnValue.PushEnd(ZString("inputmap1.mdl"));
	returnValue.PushEnd(ZString("defaultloadout.mdl"));
	returnValue.PushEnd(ZString("hullinfo.mdl"));
	returnValue.PushEnd(ZString("hangar.mdl"));
	returnValue.PushEnd(ZString("quickchat.mdl"));

	return returnValue;
}



FILETIME GetLastRunMostRecentModifiedFile(char *lastrunFilename)
{
	ZFile lastRun(lastrunFilename);
	char buffer[100];
	int numRead = lastRun.Read(buffer, sizeof(buffer));
	buffer[numRead] = 0;
	FILETIME lastRunTime;
	ZString strBuffer(buffer);

	lastRunTime.dwHighDateTime = strtoul(strBuffer.LeftOf(":"), NULL, 10);
	lastRunTime.dwLowDateTime = strtoul(strBuffer.RightOf(":"), NULL, 10);

	return lastRunTime;
}

int main(int argc, char **argv)
{
	FILETIME lastRunMostRecentModifiedFileTime = GetLastRunMostRecentModifiedFile("artwork_hash_generator_lastrun.txt");


	if (argc != 3)
	{
		printf("Usage: ArtworkHashGenerator <Artwork Path> <Output CPP Filename>\n");
		return(-1);
	}

	ZString artworkPath;
	ZString artworkSearchPath = argv[1];
	ZString outputFilename = argv[2];

	if (artworkSearchPath[artworkSearchPath.GetLength()] != '\\')
		artworkSearchPath += "\\";

	artworkPath = artworkSearchPath;

	artworkSearchPath += "*.*";

	FILETIME mostRecentFileModificationTime = ZFile::GetMostRecentFileModificationTime(artworkSearchPath);

	if (CompareFileTime(&mostRecentFileModificationTime, &lastRunMostRecentModifiedFileTime) == 0)
	{
		printf("No new modified files in the target directory. Nothing to do.\n");
		return 0;
	}
	else
	{
		ZFile lastrunFile("artwork_hash_generator_lastrun.txt", OF_WRITE | OF_CREATE);
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "%u:%u", mostRecentFileModificationTime.dwHighDateTime, mostRecentFileModificationTime.dwLowDateTime);
		
		lastrunFile.WriteString(buffer);
	}
	

	//printf("%s - %s\n", (PCC)artworkPath, (PCC)outputFilename);

	ZFile outputFile(outputFilename, OF_WRITE | OF_CREATE);

	outputFile.Write("\
// This file was automatically generated by ArtworkHashGenerator. Do not modify this file by hand. \n\n\n\
#include \"pch.h\"									\n\
													\n\
FileHashTable::FileHashTable()						\n\
{													\n\
");

	HANDLE hFind;
	WIN32_FIND_DATAA findFileData;

	TVector<ZString, DefaultEquals, DefaultCompare> whitelist = GetWhiteList();

	hFind = FindFirstFileA(artworkSearchPath, &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		//still have the default in the main directory
		printf("Invalid handle value (%d)\n", GetLastError());
		return -1;
	}
	do
	{
		if ((findFileData.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
			continue;

		if (strlen(findFileData.cFileName) > 0 && findFileData.cFileName[0] == '.')
			continue;

		ZString filename(findFileData.cFileName);
		filename = filename.ToLower();
		
		if (filename.Find("inv") == 0 && filename.Right(4) == ".mdl")
			continue;

		if (filename.Right(4) == ".png")
			continue;

		if (filename.Right(4) == ".ogg")
			continue;

		if (filename.Right(4) == ".wav")
			continue;

		if (filename.Right(7) == "bmp.mdl")
			continue;

		if (whitelist.Find(filename) > -1)
			continue;


		ZFile zFile(artworkPath + findFileData.cFileName, OF_READ | OF_SHARE_DENY_WRITE);
		ZString hash = zFile.GetSha1Hash();

		char outputLine[1024];
		sprintf_s(outputLine, "	m_filehashes.PushEnd(FileHash(\"%s\", \"%s\"));\n", 
			findFileData.cFileName, 
			(PCC)hash);

		outputFile.Write(outputLine);

		//printf("%s - %s\n", findFileData.cFileName, (PCC) hash);

	} while (FindNextFileA(hFind, &findFileData));

	outputFile.Write("}\n");

	outputFile.Release();

    return 0;
}

