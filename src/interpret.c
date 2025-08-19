#include <string.h>
#include <stdlib.h>
#include "ttf.h"

#define F26Dot6 (1 << 6)

struct stack {
	uint32_t *data;
	uint32_t size;
	uint32_t sp;
};

static void push(struct stack *stack,uint32_t data){
	//TODO : extend stack
	stack->data[stack->sp++] = data;
}

static uint32_t pop(struct stack *stack){
	if(stack->sp == 0){
		//FIXME : proper error habdling
		return 0;
	}
	return stack->data[--stack->sp];
}

static uint8_t _get_u8(uint8_t *stream,size_t *pc){
	//TODO : OOB check
	return stream[(*pc)++];
}

#define get_u8()  _get_u8(stream,&pc)
#define get_u16() (get_u8() << 8 | get_u8())
#define get_i8()  (int8_t) get_u8()
#define get_i16() (int16_t)get_u16()

int ttf_interpret(ttf_file *font,ttf_glyph *glyph,uint8_t *stream,size_t stream_size){
	struct stack stack = {.data = malloc(sizeof(uint32_t)*256),.size = 256,.sp=0};
	if(!stack.data)return -1;

	size_t pc =  0;
	while(pc < stream_size){
		uint8_t op = get_u8();
		uint8_t n;
		uint32_t data,data2,data3;
		switch(op){

		//imdediate
		case 0x40: //npushb
			n = get_u8();
			for(size_t i=0; i<n; i++){
				push(&stack,get_i8());
			}
			break;
		case 0x41: //npushw
			n = get_u8();
			for(size_t i=0; i<n; i++){
				push(&stack,get_i16());
			}
			break;
		case 0xb0:
		case 0xb1:
		case 0xb2:
		case 0xb3:
		case 0xb4:
		case 0xb5:
		case 0xb6:
		case 0xb7: //push bytes
			for(size_t i=0; i<op - 0xb0 + 1; i++){
				push(&stack,get_i8());
			}
			break;
		case 0xb8:
		case 0xb9:
		case 0xba:
		case 0xbb:
		case 0xbc:
		case 0xbd:
		case 0xbe:
		case 0xbf: //push words
			for(size_t i=0; i<op - 0xb8 + 1; i++){
				push(&stack,get_i16());
			}
			break;

		//stack manipluation
		case 0x20: //dup
			data = pop(&stack);
			push(&stack,data);
			push(&stack,data);
			break;
		case 0x21: //pop
			pop(&stack);
			break;
		case 0x22: //clear
			stack.sp = 0;
			break;
		case 0x23: //swap
			data  = pop(&stack);
			data2 = pop(&stack);
			push(&stack,data);
			push(&stack,data2);
			break;
		case 0x24: //depth
			push(&stack,stack.sp);
			break;
		case 0x25:; //cindex
			uint32_t index = pop(&stack);
			if(index > stack.sp || index == 0){
				goto error;
			}
			push(&stack,stack.data[stack.sp-index]);
			break;
		case 0x26:; //mindex
			index = pop(&stack);
			if(index > stack.sp || index == 0){
				goto error;
			}
			push(&stack,stack.data[stack.sp-index]);
			//now shift the stack the remove the old one
			memmove(&stack.data[stack.sp-index-1],&stack.data[stack.sp-index],index * sizeof(uint32_t));
			break;
		case 0x8a: //roll
			data  = pop(&stack);
			data2 = pop(&stack);
			data3 = pop(&stack);
			push(&stack,data2);
			push(&stack,data);
			push(&stack,data3);
			break;

		//control flow
		case 0x58: //if
		case 0x1b: //else
		case 0x59: //eif
			break;
		case 0x78: //jrot
		case 0x79: //jrof
			data  = pop(&stack);
			data2 = pop(&stack);
			if(op == 0x79 ? data : !data){
				pc += (int32_t)data2 - 1;
			}
			break;
		case 0x1c: //jmpr
			data  = pop(&stack);
			pc += (int32_t)data - 1;
			break;

		//logical functions (comparaison)
		case 0x50: //lt (less than)
		case 0x51: //lteq (less than or equal)
		case 0x52: //gt (greater than)
		case 0x53: //gteq (greater than or equal)
		case 0x54: //eq (equal)
		case 0x55:;//neq (not equal)
			int32_t n2 = pop(&stack);
			int32_t n  = pop(&stack);
			switch(op){
			case 0x50: //lt
				push(&stack,n < n2);
				break;
			case 0x51: //lteq
				push(&stack,n <= n2);
				break;
			case 0x52: //gt
				push(&stack,n > n2);
				break;
			case 0x53: //gteq
				push(&stack,n >= n2);
				break;
			case 0x54: //eq
				push(&stack,n == n2);
				break;
			case 0x55: //neq
				push(&stack,n != n2);
				break;
			}
			break;
		case 0x5a: //and
			push(&stack,pop(&stack) & pop(&stack);
			break;
		case 0x5b: //or
			push(&stack,pop(&stack) | pop(&stack);
			break;
		case 0x5c: //not
			push(&stack,~pop(&stack));
			break;

		//arithemtic
		case 0x60: //add
		case 0x61: //sub
		case 0x62: //div
		case 0x63: //mul
		case 0x8b: //max
		case 0x8c: //min
			n2 = pop(&stack);
			n  = pop(&stack);
			switch(op){
			case 0x60: //add
				push(&stack,n + n2);
				break;
			case 0x61: //sub
				push(&stack,n - n2);
				break;
			case 0x62: //div
				push(&stack,n * F26Dot6 / n2);
				break;
			case 0x63: //mul
				push(&stack,n * n2 / F26Dot6);
				break;
			case 0x8b: //max
				push(&stack,n > n2 ? n : n2);
				break;
			case 0x8c: //min
				push(&stack,n < n2 ? n : n2);
				break;
			}
			break;
		case 0x64: //abs
			push(&stack,abs((int32_t)pop(&stack)));
			break;
		case 0x65: //neg
			push(&stack,-(int32_t)pop(&stack));
			break;

		}
	}


	free(stack.data);
	return 0;
error:
		
	__ttf_error = "truetype bytecode error";
	return -1;
}
