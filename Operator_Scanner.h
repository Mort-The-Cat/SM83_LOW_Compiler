#ifndef OPERATOR_SCANNER
#define OPERATOR_SCANNER

#include "Syntactic_Analyser.h"

unsigned int Scan_Token_Sequence(const unsigned char* Tokens, unsigned char Token_Count, unsigned int Token_Index, const Vector* Target_Tokens)
{
	unsigned Index = 0;
	while (Index < Token_Count)
	{
		if (((Token*)(Target_Tokens->Data + Token_Index))->Token != Tokens[Index])
			return 0u;

		Index++;
		Token_Index += sizeof(Token);
	}

	// Token_Index += sizeof(Token);

	return Token_Count ? Token_Index : 0; // This is the new token index (probably)
}

typedef struct
{
	const unsigned char* Tokens; 
	unsigned char Token_Count;
	unsigned int Opcode;
	void(*Create_Function)(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode);
} Token_Pattern;

void Place_Opcode_Function(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode)
{
	ROM[(*ROM_Index)++] = Opcode;

	// That's all!
}

void Place_Operand_Opcode_Function(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode)
{
	ROM[(*ROM_Index)++] = Opcode;
	ROM[(*ROM_Index)++] = Get_Value_From_String(((Token*)(Target_Tokens->Data + (*Token_Index) - 2 * sizeof(Token)))->Representation);

	// (*Token_Index) += 4u * sizeof(Token);
}

unsigned char* Get_Representation(const Vector* Target_Tokens, unsigned int Token_Index)
{
	while (((Token*)(Target_Tokens->Data + (Token_Index)))->Token != T_HEX_LITERAL)
		Token_Index-= sizeof(Token);

	return ((Token*)(Target_Tokens->Data + (Token_Index)))->Representation;
}

void Place_High_RAM_Operand_Opcode_Function(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode)
{
	ROM[(*ROM_Index)++] = Opcode;

	ROM[(*ROM_Index)++] = Get_Value_From_String(Get_Representation(Target_Tokens, *Token_Index));
}

void Place_CB_Opcode_Function(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode)
{
	ROM[(*ROM_Index)++] = 0xCBu;	// CB Prefix
	ROM[(*ROM_Index)++] = Opcode;
}

#define Bit_Shift_INSTRUCTION(Instruction, Register, Opcode)\
	{ (const unsigned char[]){ Register, Instruction, T_NUMBER, T_SEMI }, 4, Opcode, Place_CB_Opcode_Function },\
	{ (const unsigned char[]) { Register, Instruction, T_HEX_LITERAL, T_SEMI }, 4, Opcode, Place_CB_Opcode_Function }

#define Bit_Shift_Instruction_HL(Instruction, Opcode)\
	{ (const unsigned char[]){ T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, Instruction, T_NUMBER, T_SEMI }, 6, Opcode, Place_CB_Opcode_Function },\
	{ (const unsigned char[]){ T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, Instruction, T_HEX_LITERAL, T_SEMI }, 6, Opcode, Place_CB_Opcode_Function }

#define Define_Bit_Shift_Instruction_Row(Instruction, Address)\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_B, Address + 0x00u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_C, Address + 0x01u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_D, Address + 0x02u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_E, Address + 0x03u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_H, Address + 0x04u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_L, Address + 0x05u),\
	Bit_Shift_Instruction_HL(Instruction, Address + 0x06u),\
	Bit_Shift_INSTRUCTION(Instruction, T_REGISTER_A, Address + 0x07u)

#define Define_Swap_Instruction(Register, Opcode)\
	{ (const unsigned char[]){ T_SWAP, Register, T_SEMI }, 3, Opcode, Place_CB_Opcode_Function }

#define INSTRUCTION_ROW(INSTRUCTION, Destination, Row)\
	INSTRUCTION##_INSTRUCTION(Destination, B, Row + 0u),\
	INSTRUCTION##_INSTRUCTION(Destination, C, Row + 1u),\
	INSTRUCTION##_INSTRUCTION(Destination, D, Row + 2u),\
	INSTRUCTION##_INSTRUCTION(Destination, E, Row + 3u),\
	INSTRUCTION##_INSTRUCTION(Destination, H, Row + 4u),\
	INSTRUCTION##_INSTRUCTION(Destination, L, Row + 5u),\
	INSTRUCTION##_INSTRUCTION_SOURCE_HL(Destination, Row + 6u),\
	INSTRUCTION##_INSTRUCTION(Destination, A, Row + 7u)

#define INSTRUCTION_HL_ROW(INSTRUCTION, Row)\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(B, Row + 0u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(C, Row + 1u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(D, Row + 2u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(E, Row + 3u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(H, Row + 4u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(L, Row + 5u),\
	INSTRUCTION##_INSTRUCTION_DESTINATION_HL(A, Row + 7u)


#define INSTRUCTION_DEFINER(Instruction, Address_Start)\
	INSTRUCTION_ROW(LD, B, Address_Start),\
	INSTRUCTION_ROW(LD, C, Address_Start + 0x08u),\
	INSTRUCTION_ROW(LD, D, Address_Start + 0x10u),\
	INSTRUCTION_ROW(LD, E, Address_Start + 0x18u),\
	INSTRUCTION_ROW(LD, H, Address_Start + 0x20u),\
	INSTRUCTION_ROW(LD, L, Address_Start + 0x28u),\
	INSTRUCTION_HL_ROW(LD, Address_Start + 0x30u),\
	INSTRUCTION_ROW(LD, A, Address_Start + 0x38u)

#define LD_INSTRUCTION(Destination, Source, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_##Destination, T_EQUALS, T_REGISTER_##Source, T_SEMI }, 4, Opcode, Place_Opcode_Function }

#define LD_INSTRUCTION_SOURCE_HL(Destination, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_##Destination, T_EQUALS, T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_SEMI }, 6, Opcode, Place_Opcode_Function }

#define LD_INSTRUCTION_DESTINATION_HL(Source, Opcode)\
	{ (const unsigned char[]){ T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_EQUALS, T_REGISTER_##Source, T_SEMI }, 6, Opcode, Place_Opcode_Function }

#define ALU_INSTRUCTION(Register_Operand, Operation, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_A, Operation, T_REGISTER_##Register_Operand, T_SEMI }, 4, Opcode, Place_Opcode_Function }

#define ALU_INSTRUCTION_SOURCE_HL(Operation, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_A, Operation, T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_SEMI }, 6, Opcode, Place_Opcode_Function }

#define ALU_INSTRUCTION_OPERAND(Operation, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_A, Operation, T_HEX_LITERAL, T_SEMI }, 4, Opcode, Place_Operand_Opcode_Function },\
	{ (const unsigned char[]) { T_REGISTER_A, Operation, T_NUMBER, T_SEMI }, 4, Opcode, Place_Operand_Opcode_Function }

#define ALU_INSTRUCTION_ROW(Operation, Row)\
	ALU_INSTRUCTION(B, Operation, Row),\
	ALU_INSTRUCTION(C, Operation, Row + 1),\
	ALU_INSTRUCTION(D, Operation, Row + 2),\
	ALU_INSTRUCTION(E, Operation, Row + 3),\
	ALU_INSTRUCTION(H, Operation, Row + 4),\
	ALU_INSTRUCTION(L, Operation, Row + 5),\
	ALU_INSTRUCTION_SOURCE_HL(Operation, Row + 6),\
	ALU_INSTRUCTION(A, Operation, Row + 7),\
	ALU_INSTRUCTION_OPERAND(Operation, Row + 0x46)

#define INC_DEC_INSTRUCTION(Operation, Register, Address)\
	{ (const unsigned char[]){ T_REGISTER_##Register, Operation, T_SEMI }, 3, Address, Place_Opcode_Function }

#define INC_DEC_INSTRUCTION_HL(Operation, Address)\
	{ (const unsigned char[]){ T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, Operation, T_SEMI }, 5, Address, Place_Opcode_Function }

#define INC_DEC_INSTRUCTION_DEFINER(Operation, Address)\
	INC_DEC_INSTRUCTION(Operation, B, 0x04u + Address),\
	INC_DEC_INSTRUCTION(Operation, C, 0x0Cu + Address),\
	INC_DEC_INSTRUCTION(Operation, D, 0x14u + Address),\
	INC_DEC_INSTRUCTION(Operation, E, 0x1Cu + Address),\
	INC_DEC_INSTRUCTION(Operation, H, 0x24u + Address),\
	INC_DEC_INSTRUCTION(Operation, L, 0x2Cu + Address),\
	INC_DEC_INSTRUCTION_HL(Operation, 0x34u + Address),\
	INC_DEC_INSTRUCTION(Operation, A, 0x3Cu + Address)

#define PUSH_POP_INSTRUCTION(Registers, Operation, Opcode)\
	{ (const unsigned char[]){ Operation, T_REGISTER_PAIR_##Registers, T_SEMI }, 3, Opcode, Place_Opcode_Function }

#define PUSH_POP_INSTRUCTIONS(Operation, Opcode)\
	PUSH_POP_INSTRUCTION(BC, Operation, 0xC1u + Opcode),\
	PUSH_POP_INSTRUCTION(DE, Operation, 0xD1u + Opcode),\
	PUSH_POP_INSTRUCTION(HL, Operation, 0xE1u + Opcode),\
	PUSH_POP_INSTRUCTION(AF, Operation, 0xF1u + Opcode)

#define WORD_INC_DEC_INSTRUCTION(Register_Pair, Operation, Opcode)\
	{ (const unsigned char[]){ Register_Pair, Operation, T_SEMI }, 3, Opcode, Place_Opcode_Function }

#define WORD_INC_DEC_INSTRUCTIONS(Operation, Opcode)\
	WORD_INC_DEC_INSTRUCTION(T_REGISTER_PAIR_BC, Operation, 0x03u + Opcode),\
	WORD_INC_DEC_INSTRUCTION(T_REGISTER_PAIR_DE, Operation, 0x13u + Opcode),\
	WORD_INC_DEC_INSTRUCTION(T_REGISTER_PAIR_HL, Operation, 0x23u + Opcode),\
	WORD_INC_DEC_INSTRUCTION(T_SP, Operation, 0x33u + Opcode)

#define WORD_ADD_INSTRUCTION(Register_Pair, Opcode)\
	{ (const unsigned char[]){ T_REGISTER_PAIR_HL, T_PLUS_EQUALS, Register_Pair, T_SEMI }, 4, Opcode, Place_Opcode_Function }

#define WORD_ASSIGN_INSTRUCTION(Register_Pair, Opcode)\
	{ (const unsigned char[]){ Register_Pair, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, Opcode, Place_Opcode_Word_Operand_Function },\
	{ (const unsigned char[]) { Register_Pair, T_EQUALS, T_NUMBER, T_SEMI }, 4, Opcode, Place_Opcode_Word_Operand_Function }

void Place_Opcode_Word_Operand_Function(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens, unsigned int Opcode)
{
	unsigned int Word;
	ROM[(*ROM_Index)++] = Opcode;
	Word = *Token_Index - sizeof(Token);
	while (((Token*)(Target_Tokens->Data + Word))->Token != T_HEX_LITERAL && ((Token*)(Target_Tokens->Data + Word))->Token != T_NUMBER)
		Word -= sizeof(Token);

	Word = Get_Value_From_String(((Token*)(Target_Tokens->Data + Word))->Representation);
	ROM[(*ROM_Index)++] = Word;
	ROM[(*ROM_Index)++] = Word >> 8u;
}

unsigned char Scan_Operators(unsigned char* ROM, unsigned int* Token_Index, unsigned int* ROM_Index, const Vector* Identifiers, const Vector* Target_Tokens)
{
	Token_Pattern Patterns[] =
	{
		INSTRUCTION_DEFINER(LD, 0x40u),

		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_C, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 6, 0xE2u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_REGISTER_C, T_CLOSE_SQ, T_SEMI }, 6, 0xF2u, Place_Opcode_Function },

		{ (const unsigned char[]) { T_OPEN_SQ, T_HEX_LITERAL, T_PLUS_EQUALS, T_REGISTER_C, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 8, 0xE2u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_HEX_LITERAL, T_PLUS_EQUALS, T_REGISTER_C, T_CLOSE_SQ, T_SEMI }, 8, 0xF2u, Place_Opcode_Function },

		PUSH_POP_INSTRUCTIONS(T_POP, 0x00u),
		PUSH_POP_INSTRUCTIONS(T_PUSH, 0x04u),

		WORD_INC_DEC_INSTRUCTIONS(T_PLUS_PLUS, 0x0u),
		WORD_INC_DEC_INSTRUCTIONS(T_MINUS_MINUS, 0x08u),

		WORD_ADD_INSTRUCTION(T_REGISTER_PAIR_BC, 0x09u),
		WORD_ADD_INSTRUCTION(T_REGISTER_PAIR_DE, 0x19u),
		WORD_ADD_INSTRUCTION(T_REGISTER_PAIR_HL, 0x29u),
		WORD_ADD_INSTRUCTION(T_SP, 0x39u),

		WORD_ASSIGN_INSTRUCTION(T_REGISTER_PAIR_BC, 0x01u),
		WORD_ASSIGN_INSTRUCTION(T_REGISTER_PAIR_DE, 0x11u),
		WORD_ASSIGN_INSTRUCTION(T_REGISTER_PAIR_HL, 0x21u),
		WORD_ASSIGN_INSTRUCTION(T_SP, 0x31u),

		{ (const unsigned char[]) { T_REGISTER_A, T_ROTATE_LEFT, T_NUMBER, T_SEMI }, 4, 0x07u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_CARRY_LEFT_ROTATE, T_NUMBER, T_SEMI }, 4, 0x17u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_ROTATE_RIGHT, T_NUMBER, T_SEMI }, 4, 0x0Fu, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_CARRY_RIGHT_ROTATE, T_NUMBER, T_SEMI }, 4, 0x1Fu, Place_Opcode_Function },

		Define_Bit_Shift_Instruction_Row(T_CARRY_LEFT_ROTATE, 0x10u),
		Define_Bit_Shift_Instruction_Row(T_CARRY_RIGHT_ROTATE, 0x18u),
		Define_Bit_Shift_Instruction_Row(T_ROTATE_LEFT, 0x00u),
		Define_Bit_Shift_Instruction_Row(T_ROTATE_RIGHT, 0x08u),
		Define_Bit_Shift_Instruction_Row(T_LEFT_SHIFT, 0x20u),
		Define_Bit_Shift_Instruction_Row(T_ARITHMETIC_RIGHT_SHIFT, 0x28u),
		Define_Bit_Shift_Instruction_Row(T_LOGICAL_RIGHT_SHIFT, 0x38u),

		Define_Swap_Instruction(T_REGISTER_B, 0x30u),
		Define_Swap_Instruction(T_REGISTER_C, 0x31u),
		Define_Swap_Instruction(T_REGISTER_D, 0x32u),
		Define_Swap_Instruction(T_REGISTER_E, 0x33u),
		Define_Swap_Instruction(T_REGISTER_H, 0x34u),
		Define_Swap_Instruction(T_REGISTER_L, 0x35u),
		{ (const unsigned char[]) { T_SWAP, T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_SEMI }, 5, 0x36u, Place_CB_Opcode_Function },
		Define_Swap_Instruction(T_REGISTER_A, 0x37u),

		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_REGISTER_PAIR_HL, T_PLUS_PLUS, T_CLOSE_SQ, T_SEMI }, 7, 0x2Au, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_REGISTER_PAIR_HL, T_MINUS_MINUS, T_CLOSE_SQ, T_SEMI }, 7, 0x3Au, Place_Opcode_Function },
		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_HL, T_PLUS_PLUS, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 7, 0x22u, Place_Opcode_Function  },
		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_HL, T_MINUS_MINUS, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 7, 0x32u, Place_Opcode_Function },

		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_BC, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 6, 0x02u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_DE, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 6, 0x12u, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_REGISTER_PAIR_BC, T_CLOSE_SQ, T_SEMI }, 6, 0x0Au, Place_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_REGISTER_PAIR_DE, T_CLOSE_SQ, T_SEMI }, 6, 0x1Au, Place_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_B, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x06u, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_B, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x06u, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_D, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x16u, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_D, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x16u, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_H, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x26u, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_H, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x26u, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_C, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x0Eu, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_C, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x0Eu, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_E, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x1Eu, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_E, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x1Eu, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_L, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x2Eu, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_L, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x2Eu, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 6, 0x36u, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_OPEN_SQ, T_REGISTER_PAIR_HL, T_CLOSE_SQ, T_EQUALS, T_NUMBER, T_SEMI }, 6, 0x36u, Place_Operand_Opcode_Function },

		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_HEX_LITERAL, T_SEMI }, 4, 0x3Eu, Place_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_NUMBER, T_SEMI }, 4, 0x3Eu, Place_Operand_Opcode_Function },

		INC_DEC_INSTRUCTION_DEFINER(T_PLUS_PLUS, 0x00u),
		INC_DEC_INSTRUCTION_DEFINER(T_MINUS_MINUS, 0x01u),

		ALU_INSTRUCTION_ROW(T_PLUS_EQUALS,			0x80u),
		ALU_INSTRUCTION_ROW(T_CARRY_PLUS_EQUALS,	0x88u),
		ALU_INSTRUCTION_ROW(T_MINUS_EQUALS,			0x90u),
		ALU_INSTRUCTION_ROW(T_CARRY_MINUS_EQUALS,	0x98u),
		ALU_INSTRUCTION_ROW(T_AMPERSAND_EQUALS,		0xA0u),
		ALU_INSTRUCTION_ROW(T_XOR_EQUALS,			0xA8u),
		ALU_INSTRUCTION_ROW(T_PIPE_EQUALS,			0xB0u),
		ALU_INSTRUCTION_ROW(T_COMPARE,				0xB8u),

		{ (const unsigned char[]) { T_OPEN_SQ, T_HEX_LITERAL, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 6, 0xEAu, Place_Opcode_Word_Operand_Function },
		{ (const unsigned char[]) { T_OPEN_SQ, T_NUMBER, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 6, 0xEAu, Place_Opcode_Word_Operand_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_HEX_LITERAL, T_CLOSE_SQ, T_SEMI }, 6, 0xFAu, Place_Opcode_Word_Operand_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_NUMBER, T_CLOSE_SQ, T_SEMI }, 6, 0xFAu, Place_Opcode_Word_Operand_Function },

		{ (const unsigned char[]) { T_OPEN_SQ, T_HEX_LITERAL, T_PLUS_EQUALS, T_HEX_LITERAL, T_CLOSE_SQ, T_EQUALS, T_REGISTER_A, T_SEMI }, 8, 0xE0u, Place_High_RAM_Operand_Opcode_Function },
		{ (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_OPEN_SQ, T_HEX_LITERAL, T_PLUS_EQUALS, T_HEX_LITERAL, T_CLOSE_SQ, T_SEMI }, 8, 0xF0u, Place_High_RAM_Operand_Opcode_Function },

		// { (const unsigned char[]) { T_REGISTER_A, T_EQUALS, T_REGISTER_B, T_SEMI }, 4, }
	};

	unsigned int Patterns_Count = sizeof(Patterns) / sizeof(Token_Pattern);

	unsigned int Pattern_Index = 0;

	unsigned int Token_Sequence = 0;

	// Token_Index is the index of the current token in the byte-code generation
	// ROM_Index is the current position of the write cursor in ROM
	// ROM is the ROM data itself 0x0000 - 0x7FFF
	// Identifiers are the list of local identifiers (Byte_Code)
	// Target_Tokens are the list of Tokens we're parsing

	while (Pattern_Index < Patterns_Count)
	{
		Token_Sequence = Scan_Token_Sequence(Patterns[Pattern_Index].Tokens, Patterns[Pattern_Index].Token_Count, *Token_Index, Target_Tokens);

		if (Token_Sequence)
		{
			*Token_Index = Token_Sequence;
			Patterns[Pattern_Index].Create_Function(ROM, Token_Index,ROM_Index, Identifiers, Target_Tokens, Patterns[Pattern_Index].Opcode);
			return 0;
		}

		Pattern_Index++;
	}

	return 1;
}

#endif