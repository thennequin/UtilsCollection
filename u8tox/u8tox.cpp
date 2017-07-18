// Convert cpp unicode string format to to ASCII
// "\uf1fb" => "\xEF\x87\xBB"

#include <stdio.h>
#include <stdlib.h>

#include <string>

bool IsXDigit(char cChar)
{
	return (cChar >= '0' && cChar <= '9') || ((cChar & ~' ') >= 'A' && (cChar & ~' ') <= 'F') || ((cChar & ~' ') >= 'a' && (cChar & ~' ') <= 'f');
}

void main(int argc, char** argv)
{
	if( argc >= 2 )
	{
		FILE* pFile = fopen(argv[1], "rb");
		if (pFile == NULL)
		{
			printf("invalid input file");
			exit(1);
			return;
		}

		fseek(pFile, 0, SEEK_END);
		size_t iSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		unsigned char* pString = (unsigned char*)malloc(iSize+1);
		fread(pString, 1, iSize, pFile);
		pString[iSize] = '\0';

		FILE* pOut = stdout;
		if (argc == 3)
		{
			pOut = fopen(argv[2], "wb");
			if (pOut == NULL)
			{
				printf("invalid output file");
				fclose(pFile);
				exit(1);
				return;
			}
		}

		const unsigned char* pChars = pString;
		while ( *pChars != '\0' )
		{
			if( *pChars == '\\' 
				&& *(pChars+1) == 'u' 
				&& IsXDigit(*(pChars+2))
				&& IsXDigit(*(pChars+3))
				&& IsXDigit(*(pChars+4))
				&& IsXDigit(*(pChars+5)) )
			{
				std::string sUnicode = "XXXX";
				sUnicode[0] = *(pChars+2);
				sUnicode[1] = *(pChars+3);
				sUnicode[2] = *(pChars+4);
				sUnicode[3] = *(pChars+5);
				unsigned int iUnicode = std::stoul(sUnicode, NULL, 16);

				//printf("%d", iUnicode);
				if (iUnicode < 0x0080)
				{
					fprintf(pOut, "\\x%02x", (int)iUnicode);
				}
				else if (iUnicode >= 0x80 && iUnicode <= 0x800)
				{
					fprintf(pOut, "\\x%2x\\x%2x", (int)(0xC0 | (iUnicode >> 6)), (int)(0x80 | (iUnicode & 0x3F)));
				}
				else if (iUnicode >= 0x800 && iUnicode <= 0x7FFF)
				{
					fprintf(pOut, "\\x%2x\\x%2x\\x%2x", (char)(0xE0 | (iUnicode >> 12)), (char)(0x80 | ((iUnicode >> 6) & 0x3F)), (char)(0x80 | (iUnicode & 0x3F)));
				}
				else if (iUnicode >= 0x8000 && iUnicode <= 0x7FFFF)
				{
					int iVal = (int)(0xF0 | (iUnicode >> 18) & 0x07);
					int iVal2 = (int)(0xE0 | ((iUnicode >> 12) & 0x3F));
					fprintf(pOut, "\\x%2x\\x%2x\\x%2x\\x%2x", (int)(0xF0 | (iUnicode >> 18)), (int)(0x80 | ((iUnicode >> 12) & 0x3F)),	(int)(0x80 | ((iUnicode >> 6) & 0x3F)), (int)(0x80 | (iUnicode & 0x3F)));
					//fprintf(pOut, "\\x%2X\\x%2X\\x%2X", (int)(0xE0 | ((iUnicode >> 12) & 0x3F)), (int)(0x80 | ((iUnicode >> 6) & 0x3F)), (int)(0x80 | (iUnicode & 0x3F)));
				}

				pChars += 6;
			}
			else
			{
				fputc(*pChars, pOut);
				++pChars;
			}
		}
		exit(0);
	}
	else
	{
		printf("Usage: u8tox <input cpp File> <optional output cpp file>\nIf no output file setted, print result in console");
	}
}