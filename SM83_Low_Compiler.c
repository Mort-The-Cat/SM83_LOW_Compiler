#include <stdio.h>

#include "Realloc_Vector.h"
#include "Tokens.h"
#include "Syntactic_Analyser.h"

void Output_Tokens()
{
	Token T;
	Vector Tokens = { 0, 0, 0 };

	Tokenise(&Tokens, "Test_LOW_Program.low");

	Place_Identifiers(&Tokens);

	for (unsigned long W = 0; W < Tokens.Size; W+=sizeof(Token))
	{
		T = *(Token*)(Tokens.Data + W);
		if (T.Token == T_ID)
			printf("\\");

		if (T.Token == T_OPEN_SCOPE)
			printf("\n");
		printf("%s ", T.Representation);
		if (T.Token == T_OPEN_SCOPE || T.Token == T_CLOSE_SCOPE || T.Token == T_SEMI)
			printf("\n");
	}

	Generate_Byte_Code(&Tokens);

	Vector_Clear(&Tokens);
}

int main(unsigned char Argument_Count, char** Argument_String)
{
	/*while (--Argument_Count) // Argument_String[0] is the executable filename, so we don't want to include that
	{
		printf("%s\n", Argument_String[Argument_Count]);
	}*/

	Output_Tokens();

	getchar();

	/*const char* Hello_World = "Hello, world!";

	Vector Buffer = {0, 0, 0};

	Vector_From_String(&Buffer, Hello_World);

	Buffer.Data[Buffer.Size - 1] = ' ';
	//Vector_Pop(&Buffer, 1u); // Get rid of the early NULL-termination (we want to add another string here
	Vector_From_String(&Buffer, Argument_String[0]);

	printf("%s", Buffer.Data);*/

	return 0;
}