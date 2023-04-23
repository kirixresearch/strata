/*!
 *
 * Copyright (c) 2009-2013, Kirix Research, LLC.  All rights reserved.
 *
 * Project:  Standard Library
 * Author:   Benjamin I. Williams
 * Created:  2009-06-03
 *
 */


#ifndef H_KL_BASE64_H
#define H_KL_BASE64_H


namespace kl
{


typedef enum
{
    step_A, step_B, step_C
} base64_encodestep;

typedef struct
{
    base64_encodestep step;
    char result;
    int stepcount;
} base64_encodestate;

typedef enum
{
    step_a, step_b, step_c, step_d
} base64_decodestep;

typedef struct
{
    base64_decodestep step;
    char plainchar;
} base64_decodestate;

void base64_init_encodestate(base64_encodestate* state_in);
char base64_encode_value(char value_in);
int base64_encode_block(const char* plaintext_in,
                        int length_in,
                        char* code_out,
                        base64_encodestate* state_in);
int base64_encode_blockend(char* code_out, base64_encodestate* state_in);

void base64_init_decodestate(base64_decodestate* state_in);
int base64_decode_value(char value_in);
int base64_decode_block(const char* code_in,
                        const int length_in,
                        char* plaintext_out,
                        base64_decodestate* state_in);

class base64_encoder
{
public:    
    base64_encoder(int buffersize_in = 4096) : _buffersize(buffersize_in)
    {
        base64_init_encodestate(&_state);
    }
    
    int encode(char value_in)
    {
        return base64_encode_value(value_in);
    }
    
    int encode(const char* code_in, const int length_in, char* plaintext_out)
    {
        return base64_encode_block(code_in, length_in, plaintext_out, &_state);
    }
    
    int encode_end(char* plaintext_out)
    {
        return base64_encode_blockend(plaintext_out, &_state);
    }
    
public:

    base64_encodestate _state;
    int _buffersize;
};


class base64_decoder
{

public:
    
    base64_decoder(int buffersize_in = 4096) : _buffersize(buffersize_in)
    {
        base64_init_decodestate(&_state);
    }
    
    int decode(char value_in)
    {
        return base64_decode_value(value_in);
    }
    
    int decode(const char* code_in, const int length_in, char* plaintext_out)
    {
        return base64_decode_block(code_in, length_in, plaintext_out, &_state);
    }
    
public:

    base64_decodestate _state;
    int _buffersize;
};





};



#endif
