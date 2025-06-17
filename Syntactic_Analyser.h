#ifndef SYNTACTIC_ANALYSER
#define SYNTACTIC_ANALYSER

#pragma warning(disable : 4996)

#include "Realloc_Vector.h"
#include "Tokens.h"
#include "Operator_Scanner.h"

typedef struct
{
	char* Name;			// This is the name of symbol
	char* Value; // values of 0x8000 or greater are RAM and aren't valid addresses for symbols
	unsigned char Token;
} Symbol;

void Remove_New_Line_Char_From_String(unsigned char* String)
{
	unsigned char Index = 0xFFu;

	while (String[Index] != '\n' && Index) Index--;

	String[Index] = '\0';
}

void Write_ROM_To_File(unsigned char* ROM)
{
	unsigned char Buffer[0xFFu];

	unsigned char Index = 0xFFu;

	FILE* File;

	printf("\n\n >> A ROM was successfully generated from the tokenised code!\n\nPlease enter an address to output to:\n");

	fgets((char*)Buffer, 0xFFu, stdin);

	Remove_New_Line_Char_From_String(Buffer);

	File = fopen((char*)Buffer, "wb");

	while (File == NULL)
	{
		printf("\n\n You seem to have given a rubbish file directory, please try again:\n");

		fgets((char*)Buffer, 0xFFu, stdin);
		Remove_New_Line_Char_From_String(Buffer);
		File = fopen((char*)Buffer, "wb");
	}

	fwrite(ROM, sizeof(unsigned char), 0x8000u, File);

	fclose(File);

	printf("File successfully written to!\n");
}

void Handle_Identifiers(Vector* Target_Tokens, unsigned int Index, const Vector* Identifiers)
{
	Symbol S;
	unsigned int Identifier_Index = 0;

	for (; Identifier_Index < Identifiers->Size; Identifier_Index += sizeof(Symbol))
	{
		S = *(Symbol*)(Identifiers->Data + Identifier_Index);
		if (!strcmp(((Token*)(Target_Tokens->Data + Index))->Representation, S.Name))
		{
			//free(((Token*)(Target_Tokens->Data + Index))->Representation);
			((Token*)(Target_Tokens->Data + Index))->Representation = S.Value;
			((Token*)(Target_Tokens->Data + Index))->Token = S.Token;
			return;
		}
	}
}

void Place_Identifiers(Vector* Target_Tokens)
{
	Vector Identifiers = { 0, 0, 0 };

	Symbol New_Symbol;

	unsigned int Local_Identifier_Count = 0;

	unsigned int Scope = 0;

	unsigned int Index = 0;

	Token* T;

	while (Index < Target_Tokens->Size)
	{
		T = (Token*)(Target_Tokens->Data + Index);

		switch (T->Token)
		{
		case T_OPEN_SCOPE:
			Scope = 1;
			break;

		case T_CLOSE_SCOPE:
			Scope = 0;
			Vector_Pop(&Identifiers, sizeof(Symbol) * Local_Identifier_Count);
			Local_Identifier_Count = 0;
			break;

		case T_DATA:
		case T_SUBROUTINE:
			if (T[2].Token == T_HEX_LITERAL || T[2].Token == T_NUMBER)
			{
				New_Symbol.Name = T[1].Representation;
				New_Symbol.Value = T[2].Representation;
				New_Symbol.Token = T[2].Token;

				Vector_Push_Memory(&Identifiers, (unsigned char*) & New_Symbol, sizeof(Symbol));
			}
			break;

		case T_ID:
			Handle_Identifiers(Target_Tokens, Index, &Identifiers);
			break;

		case T_BYTE:
		case T_WORD:
			New_Symbol.Name = T[1].Representation;
			New_Symbol.Value = T[3].Representation;
			New_Symbol.Token = T[3].Token;

			Local_Identifier_Count++;

			Vector_Push_Memory(&Identifiers, (unsigned char*) & New_Symbol, sizeof(Symbol));

			break;

		default:
			break;
		};

		Index += sizeof(Token);
	}

	// Count for local identifiers
	// Vector for all identifiers

	// If an identifier appears, check if we have it,
	// Otherwise, keep it there for the compiler to handle later

	// If we do have it, use the value and move on.
}

typedef struct
{
	unsigned int Code;
	unsigned char* Identifier;
} Byte_Code;

/*typedef struct
{
	unsigned int Address;
};*/

unsigned int Get_Value_From_String(const unsigned char* String)
{
	// $ indicates hex values
	// everything else is decimal

	unsigned int Value = 0;
	unsigned int Base;

	if (String[0] == '$') // Hex
	{
		String++;

		Base = 16;
	}
	else				// decimal
	{
		Base = 10;
	}

	while (*String)
	{
		Value *= Base;
		if (String[0] >= 'A')
		{
			Value += 10 + String[0] - 'A';
		}
		else
			Value += String[0] - '0';

		String++;
	}

	return Value;
}

unsigned char Get_Identifier_Value(const Vector* Identifiers, const unsigned char* String, unsigned int* Buffer)
{
	unsigned int Identifier_Index = 0;

	while (Identifier_Index < Identifiers->Size)
	{
		if (!strcmp((char*)((Byte_Code*)(Identifiers->Data + Identifier_Index))->Identifier, (char*)String))
		{
			*Buffer = ((Byte_Code*)(Identifiers->Data + Identifier_Index))->Code;

			return 1u;
		}

		Identifier_Index += sizeof(Byte_Code);
	}

	return 0u;
}

void Generate_ROM_Header_Checksum(unsigned char* ROM)
{
	unsigned char X = 0;

	unsigned int Index = 0x0134u;

	while (Index <= 0x014Cu)
	{
		X = X - ROM[Index] - 1;
		Index++;
	}

	ROM[0x014Du] = X;
}

typedef struct
{
	unsigned int ROM_Index;				 // Position in ROM where jump is
	const unsigned char* Representation; // Identifier in question
} Unresolved_Jump_Relative_Identifier;

void Add_Unresolved_Jump_Relative_Identifier(Vector* Unresolved_Jump_Relative_Identifiers, unsigned int ROM_Index, const unsigned char* Representation)
{
	Unresolved_Jump_Relative_Identifier Identifier;
	Identifier.ROM_Index = ROM_Index;
	Identifier.Representation = Representation;
	Vector_Push_Memory(Unresolved_Jump_Relative_Identifiers, &Identifier, sizeof(Unresolved_Jump_Relative_Identifier));
}

void Generate_Byte_Code(const Vector* Tokens)
{
	Vector Identifiers = { 0, 0, 0 };

	Vector Unresolved_Jump_Relative_Identifiers = { 0, 0, 0 };

	Byte_Code Byte;

	Token* T;

	unsigned char Jump_Conditions = 0;

	unsigned char Flags = 0;

	unsigned char ROM[0x8000u];

	memset(ROM, 0, 0x8000u);

	unsigned int ROM_Index = 0;

	unsigned int Token_Index = 0;

	unsigned int Buffer;

	while (Token_Index < Tokens->Size)
	{
		T = (Token*)(Tokens->Data + Token_Index);
		switch (T[0].Token)
		{
		case T_DATA:
		case T_SUBROUTINE:
			if (T[2].Token != T_HEX_LITERAL)
			{
				Byte.Code = ROM_Index;
				Byte.Identifier = T[1].Representation;

				Vector_Push_Memory(&Identifiers, (unsigned char*)&Byte, sizeof(Byte_Code));
				Token_Index += sizeof(Token) * 2;
			}
			else
			{
				ROM_Index = Get_Value_From_String((unsigned char*)T[2].Representation);				// Sets address to function/data reference

				Token_Index += sizeof(Token) * 3;									// increments past function/data address
			}

			Flags = T[0].Token;													// Is function/data flag
			break;

		case T_HEX_LITERAL:
		case T_NUMBER:
			if (Flags == T_DATA)
			{
				ROM[ROM_Index++] = Get_Value_From_String((unsigned char*)T->Representation);	// Loads byte into ROM, increments address
				Token_Index += sizeof(Token);									// Increments to next token
			}

			break;

		case T_CLOSE_SCOPE:
			Vector_Clear(&Identifiers);
			Vector_Clear(&Unresolved_Jump_Relative_Identifiers);
			Token_Index += sizeof(Token);
			break;

		case T_LABEL:
			Byte.Code = ROM_Index;
			Byte.Identifier = (unsigned char*)T[1].Representation;

			Vector_Push_Memory(&Identifiers, (unsigned char*)&Byte, sizeof(Byte_Code));

			if (Unresolved_Jump_Relative_Identifiers.Size)
			{
				Jump_Conditions = 0;
				while (Jump_Conditions < (Unresolved_Jump_Relative_Identifiers.Size / sizeof(Unresolved_Jump_Relative_Identifier)))
				{
					if (0 == strcmp(((Unresolved_Jump_Relative_Identifier*)Unresolved_Jump_Relative_Identifiers.Data)[Jump_Conditions].Representation, Byte.Identifier))
					{
						// CURRENT ROM INDEX IS WHAT WE WANT!!

						Buffer = ROM_Index;
						Buffer -= (((Unresolved_Jump_Relative_Identifier*)Unresolved_Jump_Relative_Identifiers.Data)[Jump_Conditions].ROM_Index + 1);

						ROM[((Unresolved_Jump_Relative_Identifier*)Unresolved_Jump_Relative_Identifiers.Data)[Jump_Conditions].ROM_Index] = Buffer; // Good!!

						// Vector_Clear(&Unresolved_Jump_Relative_Identifiers);
					}

					Jump_Conditions++;
				}
			}

			Token_Index += sizeof(Token) * 3;
			break;

		case T_CALL:
			Jump_Conditions = 2 - (T[1].Token == T_NUMBER || T[1].Token == T_HEX_LITERAL || T[1].Token == T_ID);

			if (Jump_Conditions == 2)
			{
				if (T[2].Token == T_ID)
					Get_Identifier_Value(&Identifiers, (unsigned char*)T[2].Representation, &Buffer);
				else
					Buffer = Get_Value_From_String((unsigned char*)T[2].Representation);

				Jump_Conditions = 0xC4u;

				Jump_Conditions += 0x10u * String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"carry");
				Jump_Conditions += 0x08u * !String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"not");

				ROM[ROM_Index++] = Jump_Conditions;

				ROM[ROM_Index++] = Buffer;
				ROM[ROM_Index++] = Buffer >> 8u;

				Token_Index += sizeof(Token) * 4u;
			}
			else
			{
				if (T[1].Token == T_ID)
					Get_Identifier_Value(&Identifiers, (unsigned char*)T[1].Representation, &Buffer);
				else
					Buffer = Get_Value_From_String((unsigned char*)T[1].Representation);

				ROM[ROM_Index++] = 0xCDu;
				ROM[ROM_Index++] = Buffer;
				ROM[ROM_Index++] = Buffer >> 8u;

				Token_Index += sizeof(Token) * 3u;
			}

			break;

		case T_JUMP:
			Jump_Conditions = 2 - (T[1].Token == T_NUMBER || T[1].Token == T_HEX_LITERAL || T[1].Token == T_ID || T[1].Token == T_REGISTER_PAIR_HL);

			//if (T[Jump_Conditions].Token == T_ID)
			//{

			//}

			// If jump conditions == 2, there is some kind of condition we need to look for

			if (Jump_Conditions == 2)
			{
				if (T[2].Token == T_ID)
				{
					if (!Get_Identifier_Value(&Identifiers, (unsigned char*)T[2].Representation, &Buffer))
					{
						Add_Unresolved_Jump_Relative_Identifier(&Unresolved_Jump_Relative_Identifiers, ROM_Index + 1, (unsigned char*)T[2].Representation);

						Buffer = ROM_Index;
					}
				}
				else
					Buffer = Get_Value_From_String((unsigned char*)T[2].Representation);

				Jump_Conditions = 0x20u;

				Jump_Conditions += 0x10u * String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"carry");
				Jump_Conditions += 0x08u * !String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"not");

				if ((abs((int)Buffer - (int)ROM_Index) < 120))
				{
					ROM[ROM_Index++] = Jump_Conditions;
					ROM_Index++;
					Buffer -= ROM_Index;
					ROM[ROM_Index - 1] = (char)Buffer;
				}
				else
				{
					Jump_Conditions += 0xA2u;
					ROM[ROM_Index++] = Jump_Conditions;
					ROM[ROM_Index++] = Buffer;
					ROM[ROM_Index++] = Buffer >> 8u;
				}

				Token_Index += 4 * sizeof(Token);
			}
			else
			{
				if (T[1].Token == T_ID)
				{
					ROM[ROM_Index++] = 0x18u; // JR s8

					// Look for identifier

					if (!Get_Identifier_Value(&Identifiers, (unsigned char*)T[1].Representation, &Buffer))
					{
						Add_Unresolved_Jump_Relative_Identifier(&Unresolved_Jump_Relative_Identifiers, ROM_Index, (unsigned char*)T[1].Representation);
					}

					ROM_Index++;

					Buffer -= ROM_Index;

					ROM[ROM_Index - 1] = (char)(Buffer);
				}
				else if (T[1].Token == T_REGISTER_PAIR_HL)
				{
					ROM[ROM_Index++] = 0xE9u; // JP HL
				}
				else
				{
					
					Buffer = Get_Value_From_String((unsigned char*)T[1].Representation);
					if (abs((int)Buffer - (int)ROM_Index) < 126)
					{
						ROM[ROM_Index++] = 0x18u;			// JR s8
						ROM_Index++;
						Buffer -= ROM_Index;
						ROM[ROM_Index - 1] = (char)Buffer;
					}
					else
					{
						ROM[ROM_Index++] = 0xC3u;			// JP n16
						ROM[ROM_Index++] = Buffer;
						ROM[ROM_Index++] = Buffer >> 8u;
					}
				}

				Token_Index += 3 * sizeof(Token);
			}

			break;

		case T_RETURN:
			if (T[1].Token == T_SEMI)
			{
				ROM[ROM_Index++] = 0xC9u;
				Token_Index += 2 * sizeof(Token);
			}
			else
			{
				Jump_Conditions = 0xC0u;
				Jump_Conditions += 0x10u * String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"carry");
				Jump_Conditions += 0x08u * !String_Contains((unsigned char*)T[1].Representation, (unsigned char*)"not");

				ROM[ROM_Index++] = Jump_Conditions;
				Token_Index += 3 * sizeof(Token);
			}
			break;

		case T_RETURNI:
			ROM[ROM_Index++] = 0xD9u;
			Token_Index += 2 * sizeof(Token);
			break;

		case T_ENABLEI:
			ROM[ROM_Index++] = 0xFBu;
			Token_Index += 2 * sizeof(Token);
			break;

		case T_DISABLEI:
			ROM[ROM_Index++] = 0xF3u;
			Token_Index += 2 * sizeof(Token);
			break;

		case T_BYTE:
		case T_WORD:
			Token_Index += sizeof(Token) * 5;
			break;

		case T_BCD:
			ROM[ROM_Index++] = 0x27u; // DAA instruction
			Token_Index += sizeof(Token) * 2;
			break;

		default:

			if(Scan_Operators(ROM, &Token_Index, &ROM_Index, &Identifiers, Tokens))
				Token_Index += sizeof(Token);

			break;
		}

		//Token_Index += sizeof(Token);
	}

	Generate_ROM_Header_Checksum(ROM);

	Write_ROM_To_File(ROM);
}

#endif