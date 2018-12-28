#include "msstd.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int ms_string_strip(char s[], char c)
{ 
    int i = 0;
    int j = 0; 
    for (i = 0, j = 0; s[i] != '\0'; i++) 
    { 
        if (s[i] != c) 
        { 
            s[j++] = s[i]; 
        } 
    } 
    s[j] = '\0';
	
	return 0;
} 
static char base64chars[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
/*
 * Name: base64encode()
 *
 * Description: Encodes a buffer using BASE64.
 */
void ms_base64_encode(unsigned char *from, char *to, int len)
{
	int i, j;
	unsigned char current;

	for ( i = 0, j = 0 ; i < len ; i += 3 )
	{
		current = (from[i] >> 2) ;
		current &= (unsigned char)0x3F;
		to[j++] = base64chars[(int)current];

		current = ( (unsigned char)(from[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
		if ( i + 1 >= len )
		{
			to[j++] = base64chars[(int)current];
			to[j++] = '=';
			to[j++] = '=';
			break;
		}
		current |= ( (unsigned char)(from[i+1] >> 4) ) & ( (unsigned char) 0x0F );
		to[j++] = base64chars[(int)current];

		current = ( (unsigned char)(from[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
		if ( i + 2 >= len )
		{
			to[j++] = base64chars[(int)current];
			to[j++] = '=';
			break;
		}
		current |= ( (unsigned char)(from[i+2] >> 6) ) & ( (unsigned char) 0x03 );
		to[j++] = base64chars[(int)current];

		current = ( (unsigned char)from[i+2] ) & ( (unsigned char)0x3F ) ;
		to[j++] = base64chars[(int)current];
	}
	to[j] = '\0';
	return ;	
}

int ms_base64_decode( const unsigned char * base64, unsigned char * bindata, int bindata_len)
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64chars[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64chars[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64chars[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64chars[k] == base64[i+3] )
                temp[3]= k;
        }
		if(j == bindata_len-1)
			break;
        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
		if(j == bindata_len-1)
			break;
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
		if(j == bindata_len-1)
			break;
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
			if(j == bindata_len-1)
			break;
    }
    return j;
}

static char BI_RM[] = "0123456789abcdefghijklmnopqrstuvwxyz";
int Base64to16( char *s,char *ret) 
{
        int i,j;
        int k = 0;
        int v,slop;
        int l=0;
        for (i = 0; i < strlen(s); ++i)
        {
            if (s[i] == '=') {
                break;
                
                }
            for(j=0;j<64;j++){
                if(base64chars[j]==s[i])
                    v=j;
            }
            if (v < 0) continue;
            if (k == 0)
            {
                ret[l]= BI_RM[(v >> 2)];
                slop = v & 3;
                k = 1;
            }
            else if (k == 1)
            {
                ret[l]=  BI_RM[((slop << 2) | (v >> 4))];
                slop = v & 0xf;
                k = 2;
            }
            else if (k == 2)
            {
                ret[l++]=  BI_RM[slop];
                ret[l]=  BI_RM[(v >> 2)];
                slop = v & 3;
                k = 3;
            }
            else
            {
                ret[l++]=  BI_RM[((slop << 2) | (v >> 4))];
                ret[l]=  BI_RM[(v & 0xf)];
                k = 0;
            }
            if(i<strlen(s))
                l++;
        }
        if (k == 1) ret[l]=  BI_RM[(slop << 2)];
        return l;
 }

char* hex_2_base64(char *_hex)
{
  char *hex_2_bin[16] = { "0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };
  char *dec_2_base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  //allocating memory for binary string
  int bin_size = strlen(_hex) * 4;
  while (bin_size % 6 != 0) //add space for zero padding
    bin_size += 8;
  char *bin = malloc(bin_size + 1);
  memset(bin, 0, bin_size + 1);

  //these are for strtol, its arguments need the zero terminator
  char buf[2] = { 0 };
  char b64buf[6 + 1] = { 0 };

  //converting hex input to binary
  char *bin_end = bin;
  int i = 0;
  for ( i;i < strlen(_hex); i++)
  {
    buf[0] = _hex[i];
    memcpy(bin_end, hex_2_bin[strtol(buf, NULL, 16)], 4);
    bin_end += 4;
  }

  //pad binary string w/ zeroes
  while (strlen(bin) < bin_size)
    strcat(bin, "00000000");

  //allocating memory for b64 output
  int b64size = (strlen(bin) / 6) + 1;
  char *out = malloc(b64size);
  memset(out, 0, b64size);

  //walk through binary string, converting chunks of 6 bytes into base64 chars
  char *bin_ptr = bin;
  char *out_end = out;
  int index_b64;
  while (*bin_ptr)
  {
    strncpy(b64buf, bin_ptr, 6);
    index_b64 = strtol(b64buf, NULL, 2);
    if (index_b64 == 0)
      buf[0] = '=';
    else
      buf[0] = dec_2_base64[index_b64];
    memcpy(out_end, buf, 1);
    out_end += 1;
    bin_ptr += 6;
  }

  free(bin);
  return out;
}

int url_decode(char *dst, const char *src, int len)
{
    int a=0, b=0;
    int c;

    while (b<len)
	{
        if (src[b]=='+')
		{
            dst[a++]=' ';
            b++;
        }
        else if (src[b]=='%')
		{
            if (sscanf(&src[b+1],"%2x",&c)>0)
        	{
				dst[a++]=c;
        	}
            b+=3;
        }
        else
		{
			dst[a++]=src[b++];	
		}
	}
    dst[a++]=0;
    return (a);
}
int url_encode(char *dst, const char *src, int len)
{
    int a=0, b=0;
    int c;

    while (b<len)
	{
        if (src[b] == ' ')
		{
            dst[a++] = '%';  
            dst[a++] = '2';  
            dst[a++] = '0';  
            b++;
        }
        else if (src[b] == '\n')
		{
			dst[a++] = '%';  
            dst[a++] = '0';  
            dst[a++] = 'a';  
            b++;
        }
		else if(src[b] == '+')
		{
			dst[a++] = '%';  
            dst[a++] = '2';  
            dst[a++] = 'B';  
            b++;
        }
        else
		{
			dst[a++]=src[b++];	
		}
	}
    dst[a++]=0;
    return (a);
}
int sqa_encode(char *dst, const char *src, int len)
{
	int a=0, b=0;
    int c;

    while (b<len)
	{
        if (src[b] == '&')
		{
            dst[a++] = '%';  
            dst[a++] = '2';  
            dst[a++] = '6';  
            b++;
        }
		else if(src[b] == '%')
		{
            dst[a++] = '%';  
            dst[a++] = '2';  
            dst[a++] = '5';  
            b++;
        }
		else if(src[b] == '<')
		{
            dst[a++] = '%';  
            dst[a++] = '3';  
            dst[a++] = 'C';  
            b++;
        }
		else if(src[b] == '>')
		{
            dst[a++] = '%';  
            dst[a++] = '3';  
            dst[a++] = 'E';  
            b++;
        }
        else
		{
			dst[a++]=src[b++];	
		}
	}
    dst[a++]=0;
    return (a);
}
//////////////////////////////////////////////////////////////////
#if 0
struct ms_uri_note
{
	char key[128];
	char value[128];
	AST_LIST_ENTRY(ms_uri_note) list;
};
static AST_LIST_HEAD_NOLOCK_STATIC(uri_list, ms_uri_note);

static char hex_ascii[256] = {
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
	0x08, 0x09,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1, 
	  -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1
};

static int ascii_to_hex( const unsigned char *data )
{
	return ( (int)(hex_ascii[*data]<<4) | (int)hex_ascii[*(data+1)] );
}

int ms_uri_remove()
{
	
}

int ms_uri_parse(char *data)
{
	int nTemp = 0;
	static char *zero = "\0";
	char *buf, *buf_end;
	register char chbuf;
	int cmd_count = 0;

	if (data == NULL)
		return -1;
	printf("req->query = %s\n", data);
	buf = (char *) malloc(MAX_CMD_LENGTH);
	if (buf == NULL)
		return -1;
	buf_end = buf+MAX_CMD_LENGTH-1;
	
	do  {
		int hex;
		chbuf = *data++;
		
		switch ( chbuf )
		{
		case '+':
			chbuf = ' ';
			break;
		case '%':
			hex = ascii_to_hex((unsigned char*)data);
			if ( hex > 0 ) {
				chbuf = (char)hex;
				data += 2;
			}
			break;
		case '\0':
		case '\n':
		case '\r':
		case ' ':
			*buf = '\0';
			cmd_count++;
			//end
			return 0;
		}
		
		switch ( chbuf )
		{
		case '&':
			cmd_count++;
			*buf++ = '\0';
			req->cmd_arg[cmd_count].name = buf;
			req->cmd_arg[cmd_count].value = zero;
			nTemp = 0;
			break;
		case '=':
			if(nTemp == 1){
				*buf++ = chbuf;
			}else{
				*buf++ = '\0';
				req->cmd_arg[cmd_count].value = buf;
			}
			nTemp = 1;
			break;
		default:
			*buf++ = chbuf;
			break;
		}
	} while (buf < buf_end);
	*buf = '\0';
	cmd_count++;

	return 0;
}
#endif

