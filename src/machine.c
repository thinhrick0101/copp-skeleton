#include <stdio.h>  // for getc, printf
#include <stdlib.h> // malloc, free
#include "ijvm.h"
#include "util.h" // read this in for debug prints, endianness helper functions

// see ijvm.h for descriptions of the below functions
#define STACK_SIZE 1000
FILE *in;  // use fgetc(in) to get a character from in.
           // This will return EOF if no char is available.
FILE *out; // use for example fprintf(out, "%c", value); to print value to out
byte_t *text = NULL;
unsigned int text_size = 0;
unsigned int program_counter = 0;
word_t *constants = NULL;
word_t stack[STACK_SIZE];
int stack_pointer = -1;
int lv = 0;            // local frame pointer
bool WIDE_EXT = false; // WIDE instruction

void push(signed int value)
{
  if (stack_pointer < STACK_SIZE - 1)
  {
    stack_pointer++;
    stack[stack_pointer] = value;
  }
  else
  {
    printf("Stack Overflow!\n");
  }
}

word_t pop()
{
  if (stack_pointer >= 0)
  {
    word_t value = stack[stack_pointer];
    stack_pointer--;
    return value;
  }
  else
  {
    printf("Stack Underflow!\n");
    return 0; // or some other sentinel value
  }
}

word_t top()
{
  if (stack_pointer >= 0)
  {
    return stack[stack_pointer];
  }
  else
  {
    printf("Stack is empty!\n");
    return 0; // or some other sentinel value
  }
}

void set_input(FILE *fp)
{
  in = fp;
}

void set_output(FILE *fp)
{
  out = fp;
}

int init_ijvm(char *binary_path)
{
  in = stdin;
  out = stdout;
  in = fopen(binary_path, "rb");
  if (in == NULL)
  {
    return -1; // Failed to open the binary in
  }

  // Read the magic number
  word_t magic_number;
  fread(&magic_number, sizeof(word_t), 1, in);
  magic_number = swap_uint32(magic_number);
  if (magic_number != MAGIC_NUMBER)
  {
    fclose(in);
    return -1; // Invalid binary in
  }

  // Skip the constant pool origin
  fseek(in, 4, SEEK_CUR);

  // Read the constant pool size
  word_t constant_pool_size;
  fread(&constant_pool_size, sizeof(word_t), 1, in);
  constant_pool_size = swap_uint32(constant_pool_size);
  int num_constant = constant_pool_size;
  num_constant = num_constant / 4;
  // Allocate memory for the constant pool
  constants = (word_t *)malloc(num_constant * sizeof(word_t));
  for (int i = 0; i < num_constant; i++)
  {
    fread(&constants[i], sizeof(word_t), 1, in);
    constants[i] = swap_uint32(constants[i]);
  }
  // Read the constant pool content

  // Skip the text origin
  fseek(in, 4, SEEK_CUR);

  // Read the text size
  fread(&text_size, sizeof(word_t), 1, in);
  text_size = swap_uint32(text_size);
  int num_text = text_size;
  // Allocate memory for the text section
  text = (byte_t *)malloc(num_text * sizeof(byte_t));
  for (int i = 0; i < num_text; i++)
  {
    // Read the text content
    fread(&text[i], sizeof(byte_t), 1, in);
  }
  return 0; // Successfully initialized IJVM
}

void destroy_ijvm(void)
{
  free(text);
  free(constants);
  program_counter = 0;
}

byte_t *get_text(void)
{
  return text;
}

unsigned int get_text_size(void)
{
  return text_size;
}

word_t get_constant(int i)
{
  return constants[i];
}

unsigned int get_program_counter(void)
{
  return program_counter;
}

word_t tos(void)
{
  // this operation should NOT pop (remove top element from stack)
  // TODO: implement me
  if (stack_pointer >= 0)
  {
    return stack[stack_pointer];
  }
  else
  {
    printf("Stack is empty!\n");
    return 0; // or some other sentinel value
  }
}

bool finished(void)
{
  if (program_counter >= text_size)
  {
    return true;
  }
  byte_t instruction = text[program_counter];
  if (instruction == OP_HALT || instruction == OP_ERR)
  {
    return true;
  }

  return false;
}

word_t get_local_variable(int i)
{
  // TODO: implement me
  return stack[lv + i];
}

int read_short()
{
  // calculate the first byte
  unsigned int temp_counter = get_program_counter();

  byte_t value1 = get_text()[temp_counter];
  // calculate the second byte
  temp_counter++;
  byte_t value2 = get_text()[temp_counter];
  int16_t temp = (value1 << 8) | value2;
  int offset = (int)temp;
  return offset;
}

void step(void)
{
  word_t instruction = text[program_counter];
  program_counter++;
  switch (instruction)
  {
  case OP_BIPUSH:
  {
    word_t value = get_instruction();
    value = (int8_t)value;
    value = (word_t)value;
    push(value);
    program_counter++;
    break;
  }
  case OP_DUP:
  {
    word_t value = tos();
    push(value);
    break;
  }
  case OP_IADD:
  {
    word_t value1 = pop();
    word_t value2 = pop();
    push(value1 + value2);
    break;
  }
  case OP_IAND:
  {
    word_t value1 = pop();
    word_t value2 = pop();
    push(value1 & value2);
    break;
  }
  case OP_IOR:
  {
    word_t value1 = pop();
    word_t value2 = pop();
    push(value1 | value2);
    break;
  }
  case OP_ISUB:
  {
    word_t value1 = pop();
    word_t value2 = pop();
    value1 = (int8_t)value1;
    value2 = (int8_t)value2;
    value1 = (word_t)value1;
    value2 = (word_t)value2;
    push(value2 - value1);
    break;
  }
  case OP_NOP:
  {
    // do nothing
    break;
  }
  case OP_POP:
  {
    pop();
    break;
  }
  case OP_SWAP:
  {
    word_t value1 = pop();
    word_t value2 = pop();
    push(value1);
    push(value2);
    break;
  }
  case OP_ERR:
  {
    printf("Something wrong here...\n");
    program_counter = text_size; // Set like that to stop the program
    break;
  }
  case OP_HALT:
  {
    program_counter = text_size;
    break;
  }
  case OP_IN:
  {
    word_t value = fgetc(in);
    push(value);
    if (value == EOF)
    {
      push(0);
    }
    break;
  }
  case OP_OUT:
  {
    word_t value = pop();
    fprintf(out, "%c", value);
    break;
  }
  case OP_GOTO:
  {
    int offset = read_short();
    program_counter += offset - 1;
    break;
  }

  case OP_IFEQ:
  {
    int16_t offset = read_short();
    word_t value = pop();
    if (value == 0)
    {
      program_counter += offset - 1;
    }
    else
    {
      program_counter += 2;
    }
    break;
  }

  case OP_IFLT:
  {
    int16_t offset = read_short();
    word_t value = pop();
    if (value < 0)
    {
      program_counter += offset - 1;
    }
    else
    {
      program_counter += 2;
    }
    break;
  }

  case OP_IF_ICMPEQ:
  {
    int16_t offset = read_short();
    word_t value2 = pop();
    word_t value1 = pop();
    if (value1 == value2)
    {
      program_counter += offset - 1;
    }
    else
    {
      program_counter += 2;
    }
    break;
  }

    // Add cases for other instructions...
  case OP_LDC_W:
  {
    int index = read_short();
    // Get the constant from the constant pool using the index
    word_t constant = get_constant(index);
    // Push the constant onto the stack
    push(constant);
    program_counter += 2;
    break;
  }
  case OP_ILOAD:
  {
    int index = get_instruction();
    int index2;
    if (WIDE_EXT == true)
    {
      index2 = read_short();
      program_counter += 2;
    }
    else
    {
      index2 = index;
      program_counter ++;
    }
    word_t value = stack[lv + index2];
    push(value);
    break;
  }
  case OP_ISTORE:
  {
    int index = get_instruction();
    int index2;
    word_t value = pop();
    value = (int8_t)value;
    value = (word_t)value;
    if (WIDE_EXT == true)
    {
      index2 = read_short();
      program_counter += 2; 
    }
    else
    {
      index2 = index;
      program_counter ++;
    }
    stack[lv + index2] = value;
    break;
  }
  case OP_IINC:
  {
    int index = get_instruction();
    int index2;
    if (WIDE_EXT == true)
    {
      index2 = read_short();
      program_counter += 2;
    }
    else
    {
      index2 = index;
      program_counter ++;
    }
    byte_t value = get_instruction();
    int value1 = (int8_t)value;
    stack[lv + index2] += value1;
    program_counter ++;
    break;
  }
  case OP_WIDE:
  {
    WIDE_EXT = true;
    step();
    WIDE_EXT = false;
    break;
  }
  case 
  default:
  {
    // Skip unrecognized bytes (arguments or data)
    break;
  }
  }
}

void run(void)
{
  while (!finished())
  {
    step();
  }
}

byte_t get_instruction(void)
{
  return get_text()[get_program_counter()];
}

// Below: methods needed by bonus assignments, see ijvm.h

// int get_call_stack_size(void)
//{
//  TODO: implement me
//   return sp;
//}

// Checks if reference is a freed heap array. Note that this assumes that
//
// bool is_heap_freed(word_t reference)
//{
// TODO: implement me
// return 0;
//}
