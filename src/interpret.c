#include <string.h>
#include <stdlib.h>
#include "ttf.h"

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
		uint32_t data,data2;
		switch(op){
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
		}
	}


	free(stack.data);
	return 0;
error:
		
	__ttf_error = "truetype bytecode error";
	return -1;
}
