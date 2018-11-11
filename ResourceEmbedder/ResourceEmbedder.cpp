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
#include "lz4hc.h"
#include "zstd.h"

enum ECompressMode
{
	E_COMPRESS_MODE_NONE = 0,
	E_COMPRESS_MODE_LZ4,
	E_COMPRESS_MODE_LZ4HC,
	E_COMPRESS_MODE_ZSTD,
};

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

void ExportFile(const char* pName, const char* pInputFilename, const char* pOutputFilename, const char* pRelativePath, ECompressMode eCompressMode)
{
	printf("Converting file %s\n", pInputFilename);

	FILE* pOriginFile = fopen(pInputFilename, "rb");

	assert(NULL != pOriginFile);

	unsigned int iOriginSize;
	unsigned int iCompressedSize = 0;

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

	if (eCompressMode == E_COMPRESS_MODE_LZ4)
	{
		int iCompressBound = LZ4_compressBound(iOriginSize);
		if (iCompressBound > 0)
		{
			char* pCompressed = (char*)malloc(iCompressBound);
			iCompressedSize = LZ4_compress_default(pData, pCompressed, iOriginSize, iCompressBound);
			
			if (iCompressedSize >= iOriginSize)
			{
				printf("Useless compression for this file\n");
				free(pCompressed);
				eCompressMode = E_COMPRESS_MODE_NONE;
			}
			else
			{
				free(pData);
				pData = pCompressed;
			}
		}
	}
	else if (eCompressMode == E_COMPRESS_MODE_LZ4HC)
	{
		int iCompressBound = LZ4_compressBound(iOriginSize);
		if (iCompressBound > 0)
		{
			char* pCompressed = (char*)malloc(iCompressBound);
			iCompressedSize = LZ4_compress_HC(pData, pCompressed, iOriginSize, iCompressBound, 5);

			if (iCompressedSize >= iOriginSize)
			{
				printf("Useless compression for this file\n");
				free(pCompressed);
				eCompressMode = E_COMPRESS_MODE_NONE;
			}
			else
			{
				free(pData);
				pData = pCompressed;
			}
		}
	}
	else if (eCompressMode == E_COMPRESS_MODE_ZSTD)
	{
		int iCompressBound = ZSTD_compressBound(iOriginSize);
		if (iCompressBound > 0)
		{
			char* pCompressed = (char*)malloc(iCompressBound);
			iCompressedSize = ZSTD_compress(pCompressed, iCompressBound, pData, iOriginSize, 11);

			if (iCompressedSize >= iOriginSize)
			{
				printf("Useless compression for this file\n");
				free(pCompressed);
				eCompressMode = E_COMPRESS_MODE_NONE;
			}
			else
			{
				free(pData);
				pData = pCompressed;
			}
		}
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

	size_t iIndent = oNamespaces.size();
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

	for (size_t iNamespace = 0; iNamespace < oNamespaces.size(); ++iNamespace)
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

	if (eCompressMode != E_COMPRESS_MODE_NONE)
	{
		if (eCompressMode == E_COMPRESS_MODE_LZ4)
		{
			fprintf(pSourceFile, "%s// Compressed with LZ4_compress_default\n", sIndentP.c_str());
		}
		else if (eCompressMode == E_COMPRESS_MODE_LZ4HC)
		{
			fprintf(pSourceFile, "%s// Compressed with LZ4_compress_HC level 5\n", sIndentP.c_str());
		}
		else if (eCompressMode == E_COMPRESS_MODE_ZSTD)
		{
			fprintf(pSourceFile, "%s// Compressed with ZSTD_compress level 11\n", sIndentP.c_str());
		}

		fprintf(pHeaderFile, "%s extern const unsigned int Size;\n", sIndentP.c_str());
		fprintf(pHeaderFile, "%s extern const unsigned int CompressedSize;\n", sIndentP.c_str());
		fprintf(pHeaderFile, "%s extern const char CompressedData[];\n", sIndentP.c_str());

		fprintf(pSourceFile, "%sconst unsigned int Size = %d;\n", sIndentP.c_str(), iOriginSize);
		fprintf(pSourceFile, "%sconst unsigned int CompressedSize = %d;\n", sIndentP.c_str(), iCompressedSize);
		fprintf(pSourceFile, "%sconst char CompressedData[] = {", sIndentP.c_str());
	}
	else
	{
		fprintf(pHeaderFile, "%s extern const unsigned int Size;\n", sIndentP.c_str());
		fprintf(pHeaderFile, "%s extern const char Data[];\n", sIndentP.c_str());

		fprintf(pSourceFile, "%sconst unsigned int Size = %d;\n", sIndentP.c_str(), iOriginSize);
		fprintf(pSourceFile, "%sconst char Data[] = {", sIndentP.c_str());
	}

	unsigned int iSize = (eCompressMode != E_COMPRESS_MODE_NONE) ? iCompressedSize : iOriginSize;
	for (size_t iPos = 0; iPos < iSize; ++iPos)
	{
		if (iPos % 16 == 0)
		{
			fprintf(pSourceFile, "\n%s", sIndentPP.c_str());
		}
		fprintf(pSourceFile, "0x%02X", (unsigned char)*(pData + iPos));
		if (iPos < (iSize - 1))
			fprintf(pSourceFile, ",");
	}
	
	fprintf(pSourceFile, "\n%s};\n", sIndentP.c_str());

	//Closing namespaces
	fprintf(pHeaderFile, "%s}\n", sIndent.c_str());
	fprintf(pSourceFile, "%s}\n", sIndent.c_str());

	for (size_t iNamespace = 0; iNamespace < oNamespaces.size(); ++iNamespace)
	{
		std::string sNamespaceIndent(oNamespaces.size() - iNamespace - 1, '\t');
		fprintf(pHeaderFile, "%s}\n", sNamespaceIndent.c_str());
		fprintf(pSourceFile, "%s}\n", sNamespaceIndent.c_str());
	}

	fprintf(pHeaderFile, "#endif // %s", sDefine.c_str());

	fclose(pHeaderFile);
	fclose(pSourceFile);

	free(pData);
}

void ScanFolder(const char* pInputFolder, const char* pOutputFolderBase, const char* pOutputRelative, ECompressMode eCompressMode)
{
	printf("Scanning folder %s\n", pInputFolder);

	std::string sFolder = pOutputFolderBase;
	if (NULL != pOutputRelative)
	{
		sFolder += "/";
		sFolder += pOutputRelative;
	}

	_mkdir(sFolder.c_str());
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
				ScanFolder(file.path, pOutputFolderBase, sOutRelative.c_str(), eCompressMode);
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

			ExportFile(file.name, file.path, sOutPath.c_str(), pOutputRelative, eCompressMode);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

void main(int argn, char** argv)
{
	const char* pInputFolder = NULL;
	const char* pOutputFolder = NULL;
	ECompressMode eCompressMode = E_COMPRESS_MODE_NONE;

	int iArg = 1;
	while (iArg < argn)
	{
		if (strcmp(argv[iArg], "-c") == 0)
		{
			eCompressMode = E_COMPRESS_MODE_LZ4;
		}
		else if (strncmp(argv[iArg], "-c=", 3) == 0)
		{
			const char* pMode = argv[iArg] + 3;
			if (strcmp(pMode, "lz4") == 0)
			{
				eCompressMode = E_COMPRESS_MODE_LZ4;
			}
			else if (strcmp(pMode, "lz4hc") == 0)
			{
				eCompressMode = E_COMPRESS_MODE_LZ4HC;
			}
			else if (strcmp(pMode, "zstd") == 0)
			{
				eCompressMode = E_COMPRESS_MODE_ZSTD;
			}
			else
			{
				printf("Invalid compress mode '%s' for argument -c=<mode>", pMode);
				return;
			}
		}
		else if (pInputFolder == NULL)
		{
			pInputFolder = argv[iArg];
		}
		else if (pOutputFolder == NULL)
		{
			pOutputFolder = argv[iArg];
		}
		else
		{
			printf("Invalid argument");
			return;
		}
		
		++iArg;
	}

	if (pInputFolder != NULL && pOutputFolder != NULL)
	{
		ScanFolder(pInputFolder, pOutputFolder, NULL, eCompressMode);
	}
	else
	{
		printf("Usage: ResourceEmbedder <input resource folder> <output cpp folder>\n");
		printf("  --c              : compress files with LZ4\n");
		printf("  --c=<algorithm>  : compress files with specific compression algorithms\n");
		printf("                      - lz4 : LZ4 (default)\n");
		printf("                      - lz4hc : LZ4 High Compression (level 5)\n");
		printf("                      - zstd : Facebook Zstd (level 11)\n");
	}

	//LeakTrackerShutdown();
}