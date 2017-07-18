// Convert folder with binary data to cpp data

#include <stdio.h>
#include <string>
#include <algorithm>
#include <direct.h>
#include <vector>
#include <limits.h>
#include <stdlib.h>

#include <assert.h>

#include "tinydir.h"

void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}

std::vector<std::string> split(const std::string& s, char seperator)
{
	std::vector<std::string> output;
	std::string::size_type prev_pos = 0, pos = 0;

	while ((pos = s.find(seperator, pos)) != std::string::npos)
	{
		std::string substring(s.substr(prev_pos, pos - prev_pos));
		output.push_back(substring);
		prev_pos = ++pos;
	}

	output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
	return output;
}

void PrintRow(FILE* dst, const unsigned char* buf, unsigned count)
{
	fprintf(dst, "\n    ");
	while (count > 1)
	{
		fprintf(dst, "0x%02X,", *buf);
		++buf;
		--count;
	}
	if (count > 0)
		fprintf(dst, "0x%02X", *buf);
}

void ExportFile(const char* pName, const char* pInputFilename, const char* pOutputFilename, const char* pRelativePath)
{
	FILE* pOriginFile = fopen(pInputFilename, "rb");

	assert(NULL != pOriginFile);

	//TODO read all pOriginFile and compress it
	unsigned int iOriginSize;
	fseek(pOriginFile, 0, SEEK_END);
	iOriginSize = (unsigned int)ftell(pOriginFile);
	fseek(pOriginFile, 0, SEEK_SET);
	char* pData = (char*)malloc(iOriginSize);
	fread(pData, 1, iOriginSize, pOriginFile);

	fclose(pOriginFile);

	if (iOriginSize == 0)
	{
		free(pData);
		return;
	}

	std::string sHeaderFilename = pOutputFilename + std::string(".h");
	std::string sSourceFilename = pOutputFilename + std::string(".cpp");
	ReplaceAll(sHeaderFilename, "/", "\\");
	ReplaceAll(sSourceFilename, "/", "\\");
	FILE* pHeaderFile = fopen(sHeaderFilename.c_str(), "w+");
	FILE* pSourceFile = fopen(sSourceFilename.c_str(), "w+");

	assert(NULL != pHeaderFile);
	assert(NULL != pSourceFile);

	std::string sRelativePath = "Resources";
	if (NULL != pRelativePath)
	{
		sRelativePath += "/";
		sRelativePath += pRelativePath;
	}
	std::vector<std::string> oNamespaces = split(sRelativePath, '/');

	int iIndent = oNamespaces.size();
	std::string sIndent(iIndent, '\t');
	std::string sIndentP(iIndent + 1, '\t');
	std::string sIndentPP(iIndent + 2, '\t');
	

	std::string sName = pName;
	ReplaceAll(sName, ".", "_");
	ReplaceAll(sName, " ", "_");

	std::string sDefine = "__RESOURCES_";
	if (NULL != pRelativePath)
	{
		sDefine += pRelativePath;
		sDefine += "_";
	}
	sDefine += sName;
	sDefine += "_H__";
	std::transform(sDefine.begin(), sDefine.end(), sDefine.begin(), ::toupper);
	ReplaceAll(sDefine, ".", "_");
	ReplaceAll(sDefine, " ", "_");
	ReplaceAll(sDefine, "/", "_");

	fprintf(pHeaderFile, "#ifndef %s\n", sDefine.c_str());
	fprintf(pHeaderFile, "#define %s\n\n", sDefine.c_str());

	fprintf(pSourceFile, "#include \"%s.h\"\n", sName.c_str());

	for (int iNamespace = 0; iNamespace < oNamespaces.size(); ++iNamespace)
	{
		std::string sNamespace = oNamespaces[iNamespace];
		ReplaceAll(sDefine, ".", "_");
		ReplaceAll(sDefine, " ", "_");
		std::string sNamespaceIndent(iNamespace, '\t');
		fprintf(pHeaderFile, "%snamespace %s \n%s{\n", sNamespaceIndent.c_str(), sNamespace.c_str(), sNamespaceIndent.c_str());
		fprintf(pSourceFile, "%snamespace %s \n%s{\n", sNamespaceIndent.c_str(), sNamespace.c_str(), sNamespaceIndent.c_str());
	}

	fprintf(pHeaderFile, "%snamespace %s \n%s{\n", sIndent.c_str(), sName.c_str(), sIndent.c_str());
	fprintf(pSourceFile, "%snamespace %s \n%s{\n", sIndent.c_str(), sName.c_str(), sIndent.c_str());

	fprintf(pHeaderFile, "%s extern const unsigned int Size;\n", sIndentP.c_str());
	fprintf(pHeaderFile, "%s extern const char Data[];\n", sIndentP.c_str());

	fprintf(pSourceFile, "%sconst unsigned int Size = %d;\n", sIndentP.c_str(), iOriginSize);
	fprintf(pSourceFile, "%sconst char Data[] = {", sIndentP.c_str());

	for (int iPos = 0; iPos < iOriginSize; ++iPos)
	{
		if (iPos % 16 == 0)
		{
			fprintf(pSourceFile, "\n%s", sIndentPP.c_str());
		}
		fprintf(pSourceFile, "0x%02X,", *(pData+iPos));
	}
	
	fprintf(pSourceFile, "\n%s};\n", sIndentP.c_str());

	//Closing namespaces
	fprintf(pHeaderFile, "%s}\n", sIndent.c_str());
	fprintf(pSourceFile, "%s}\n", sIndent.c_str());

	for (int iNamespace = oNamespaces.size()-1; iNamespace >= 0; --iNamespace)
	{
		std::string sNamespaceIndent(iNamespace, '\t');
		fprintf(pHeaderFile, "%s}\n", sNamespaceIndent.c_str());
		fprintf(pSourceFile, "%s}\n", sNamespaceIndent.c_str());
	}

	fprintf(pHeaderFile, "#endif // %s", sDefine.c_str());

	fclose(pHeaderFile);
	fclose(pSourceFile);

	free(pData);
}

void ScanFolder(const char* pInputFolder, const char* pOutputFolderBase, const char* pOutputRelative)
{
	std::string sFolder = pOutputFolderBase;
	if (NULL != pOutputRelative)
	{
		sFolder += "/";
		sFolder += pOutputRelative;
	}
	
	mkdir(sFolder.c_str());
	tinydir_dir dir;
	tinydir_open(&dir, pInputFolder);

	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		if (file.is_dir)
		{
			if (file.name != std::string(".") && file.name != std::string(".."))
			{
				std::string sOutRelative = "";
				if (NULL != pOutputRelative)
				{
					sOutRelative += pOutputRelative;
					sOutRelative += "/";
				}
				sOutRelative += file.name;
				ScanFolder(file.path, pOutputFolderBase, sOutRelative.c_str());
			}
		}
		else
		{
			std::string sOutPath = pOutputFolderBase;
			if (NULL != pOutputRelative)
			{
				sOutPath += "/";
				sOutPath += pOutputRelative;
			}
			sOutPath += "/";
			std::string sName = file.name;
			ReplaceAll(sName, ".", "_");
			ReplaceAll(sName, " ", "_");
			sOutPath += sName;

			ExportFile(file.name, file.path, sOutPath.c_str(), pOutputRelative);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

void main(int argn, char** argv)
{
	if (argn == 3)
	{
		ScanFolder(argv[1], argv[2], NULL);
	}
	else
	{
		printf("Usage: ResourceEmbedder <input resource folder> <output cpp folder>\n");
	}
}