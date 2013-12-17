%{
	#include "global.h"
	int yylex();
	void yyerror();
	int offset,len,commandDone;
%}
%token STRING
%%
line	:	/*empty*/
			|command	{ execute();};
command	:	fgCommand
			|fgCommand '&';
fgCommand:	simpleCmd
			|fgCommand '|' simpleCmd;
simpleCmd:	progInvocation inputRedirect outputRedirect;
progInvocation:	STRING args;
inputRedirect:	/*empty*/
			|'<' STRING;
outputRedirect:	/*empty*/
			|'>' STRING;
args		:/*empty*/
			|args STRING;
%%

//error handler
void yyerror()
{
	printf("Offset:%d\tThe command is not right!\n",offset);
}

//lexer
int yylex()
{	
	int flag;
	char c;
	//ignore the blank or Tab
	while(offset < len && (inputBuff[offset] == ' ' ||inputBuff[offset] =='\t'))
	{
		offset++;
	}
	
	flag=0;
	while(offset < len)//lex:return the terminal sign
	{
		c=inputBuff[offset];
		
		if(c == ' '|| c== '\t')
		{
			offset++;
			return STRING;
		}
		
		if(c=='<' ||c=='>'||c=='&')
		{
			if(flag ==1)
			{
				flag = 0;
				return STRING;
			}
			offset++;
			return c;
		}
		flag = 1;
		offset++;
	}
	
	//offset == len
	if(flag==1)
	{
		return STRING;
	}
	else
	{
		return 0;//The blank line
	}
}


int main(int argc,char **argv)
{
    int i;
    char c;

    init();
    commandDone = 0;
    
    welcomeShell();
    
    //^^^^^^^
    //printf("successfully enter!\n");

    while(1)
    {
    	printf("user-sh@%s>",get_current_dir_name());
        i=0;
        while((c=getchar())!='\n')
        {
            inputBuff[i++] = c;
        }
        inputBuff[i] = '\0';
        
        len = i;
        offset = 0;
        
        isBackground = 0;
        nowstatus = FOREGROUND;
        
        //^^^^^^^
        //printf("%s\n",inputBuff);

        //Parser
        yyparse();
        
        //^^^^^^^
        //printf("ParserOver\n");

        if(commandDone == 1)
        {
            commandDone =0;
            addHistory(inputBuffCopy);
        }
    }

    return (EXIT_SUCCESS);
}
		
			
	
	
	
	
	
	
	
	
	
	
	
	
