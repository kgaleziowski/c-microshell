#define _GNU_SOURCE         /* prawidlowe dzialanie strdup i readline */

/* BIBLIOTEKI */
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>         /* struktura do rm -R */

/* WIELKOSCI */
#define COUNTOFCOMMANDS 10
#define MAXHISTORY 100

/* KOLORY */
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define BLUE "\033[1;34m"
#define MAGENTA "\e[0;35m"
#define WHITE "\033[1;37m"
#define CYAN "\e[0;36m"
#define YELLOW "\033[01;33m"


/* ZMIENNE GLOBALNE */
char *functionalCommands[COUNTOFCOMMANDS] = {"cd","help","mkdir","cp","clear","history","pwd","login","wc","rm"};
char *history[MAXHISTORY];
int historyCount = 0;
int paramCount = 0;
char COMMAND[512],PATH[512],*PARAMETERS[20];
char *directors[100];
char *login;
char previousPATH[512];
char *home; 
char lastPath[512];
char pathForMinus[512];
char minus[512];


/* SEGMENT FUNKCJI OBSLUGUJACYCH WEJSCIE - MOJE IMPLEMENTACJE */

/* dwie funkcje skladowe funkcji printPrompt() odpowiadajace za wyswietlenie znaku zachety */

void getLogin()
{
    login = getenv("USER");
    printf(RED "[" YELLOW "*" BLUE "%s" YELLOW "*" WHITE ":",login);
}

void getCurrentDirectory()
{	
    getcwd(PATH,sizeof(PATH));
    printf(GREEN "%s" RED "]" WHITE ": ",PATH);
}

/* login - wyswietla login aktualnego uzytkownika na zawolanie, teoretycznie bezuzyteczna w przypadku wyswietlania znaku zachety 
w powyzszej postaci aczkolwiek moze byc przydatna jesli printPrompt() zostanie jakkolwiek zmodyfikowane */

void printLogin()
{
	char *login;
    login = getenv("USER");
	printf(YELLOW "Zalogowany uzytkownik:"RED" %s\n",login);
}

/* pwd - wyswietla sciezke do biezacego katalogu roboczego, teoretycznie bezuzyteczna w przypadku wyswietlania znaku zachety 
w powyzszej postaci aczkolwiek moze byc przydatna jesli printPrompt() zostanie jakkolwiek zmodyfikowane*/

void printWorkingDirectory()
{
	getcwd(PATH,sizeof(PATH));
	printf(YELLOW "Obecnie znajdujesz sie w:"RED" %s\n",PATH);
}

/* funkcja okresla czy wywolana komenda ma zostac obsluzona przez moja implementacje (jesli jest zaimplementowana przeze mnie), 
badz przez execve jesli takowej implementacji nie ma */

int mineOrShells(char *inputString)
{
	int i;
	int whoseCommand = 0;
	for(i = 0; i < COUNTOFCOMMANDS; i++)
	{
		if( strcmp( inputString,functionalCommands[i]) == 0 )
		{
			whoseCommand = 1;
			break;
		}
	}
	return whoseCommand;
}

/* segment funkcji odpowiadajcych za wyswietlanie historii */

/* funkcja zbiera linie wprowadzone do terminalu i zapisuje je, pojemnosc ograniczona przez MAXHISTORY */

void trackingHistory(char *historyStr)
{
    if(historyCount < MAXHISTORY)
    {
        history[historyCount] = strdup(historyStr);
        historyCount++;
    }
    else
    {
    	int i;
        free(history[0]);
        for(i = 1; i < MAXHISTORY; i++)
        {
            history[i-1] = history[i];
        }
        history[MAXHISTORY - 1] = strdup(historyStr);
    }
}

/* history - wyswietla, w kolejnosci od najstarszej, wpisane komendy */

void printingHistory()
{
	int i;
	    for(i = 0; i < historyCount; i = i + 2)
	        printf("%3i.    %s\n",i+1,history[i]);
}

/* cd - działaja analogicznie jak cd z basha */

void changeDirectory(char *firstParam)
{

	if(paramCount == 1)
	{
		chdir(home);
	}
    else if(strcmp(firstParam,"-") == 0)
    {
    	getcwd(minus,sizeof(minus));
    	strcpy(pathForMinus,minus);
    	chdir(lastPath);
    }
    else
    {
    	getcwd(minus,sizeof(minus));
    	strcpy(lastPath,minus);
	    if(strcmp(firstParam,"~") == 0 )
	    {
			chdir(home);
	    }
	    else if(chdir(firstParam) == -1)
			printf(RED "Katalog nieznaleziony\n");
    }
}

/* clear - czyszczenie okna terminalu */

void clearTerminal()
{
    system("clear");
}

/* cp - kopiowanie plikow w formie cp [plik zrodlowy] [plik docelowy] */

void copyingFiles(char *firstParam, char *secondParam, char *thirdParam)
{
	char* BUF[1024];
	int toCopy,coppiedFile,copyProcess;
	if( paramCount == 1 )
	    	printf(RED "Nalezy okreslic plik zrodlowy oraz plik docelowy\n");
	    else if( paramCount == 2)
	    	printf(RED "Nalezy okreslic plik docelowy\n");
	    else
		{	
		    toCopy = open(firstParam,O_RDONLY);
		   	coppiedFile = open(secondParam,O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); /* uprawnienia pliku */
		    if( toCopy == -1 )
				printf(RED "Plik ktory chcesz skopiowac nie istnieje\n");
		    else if( coppiedFile == -1 )
				printf(RED "Bledne miejsce docelowe\n");
			else
			{
				while( (copyProcess = read(toCopy,BUF,1024)) > 0 )
					write(coppiedFile,BUF,(read(toCopy,BUF,1024)));

				printf(YELLOW "Pomyslnie skopiowano plik %s do pliku %s\n",PARAMETERS[1],PARAMETERS[2]);
			}
		}
}

/* wc - liczba linii, slow, znakow analogicznie jak w bashu */

void wordCount(char *firstParam)
{
		int hmLines = 0;
		int hmWords = 0;
		int hmChars = 0;
		char charek;
		FILE *inputFile;
		inputFile = fopen(firstParam,"rw+"); /* taki zapis naprawia bug z otwieraniem folderow */

		if(paramCount == 1)
		{
			printf(RED "Nalezy okreslic plik na ktorym ma zostac uzyta komenda wc\n");
		}
		else if(inputFile == 0)
			printf(RED "Plik na ktorym chcesz uzyc komendy wc nie istnieje lub jest katalogiem!\n");
		else
		{
			while((charek = fgetc(inputFile)) != EOF)
			{
				hmChars++;
				if(charek == '\n')
				{
					hmWords++;
					hmLines++;
					hmChars--;
				}
				else if(charek == ' ')
				{
					hmWords++;
				}
			}
			printf(YELLOW "Liczba slow: %i\n",hmWords - 1);
			printf(YELLOW "Liczba znakow: %i\n",hmChars);
			printf(YELLOW "Liczba linii: %i\n",hmLines + 1);
		}
}
/*-------------------------------------------------------------------------------------------------------------------*/
/*################################################# SEGMENT REMOVE ##################################################*/
/*-------------------------------------------------------------------------------------------------------------------*/


void removeDir(char *firstParam)
{
	char tempPath[512];
	getcwd(tempPath,sizeof(tempPath));        /* zapamietuje sciezke z ktorej wywoluje rm -R */
	changeDirectory(firstParam);              /* wchodze w katalog do usuniecia */
	struct dirent *de;              /* struktura z dirent.h */

    DIR *dr = opendir(".");        /* otworzenie kazdego katalogu dla ktorego jest wywolane rm -R */

    if (dr == NULL) 
    {
        printf(RED "Folder jest niemozliwy do otworzenia!\n" );
    }
    else
    {
	    printf(BLUE "Udalo ci sie usunac folder o nazwie: %s zawierajacy nastepujace pliki: \n",firstParam);
	    while ((de = readdir(dr)) != NULL)
	    {
	    	    if (de->d_type == DT_REG)
	  			{
	     			printf("%s ", de->d_name);                
	  			}
	    		remove(de->d_name);                  /* usuwanie pojedynczych plikow z katalogu */
	    }
	    printf("\n");
	    closedir(dr); 
	    changeDirectory(tempPath);      /* wracam do sciezki z ktorej wywolalem rm */
	    rmdir(firstParam);        /* usuwam pusty katalog */
	}   
}

void removeDirsOrFiles()
{
	char letsProceed[3];
	int i;
	if (paramCount == 1)
		printf(RED "Nalezy okreslic plik ktory chcesz usunac!\n");
	else
	{
		if(strcmp(PARAMETERS[1],"-R") == 0)
		{
			if(paramCount > 2)
			{
				printf(RED "!OTRZEZENIE! " YELLOW "Komenda sugeruje ze chcesz usunac caly folder ktory prawdopodobnie zawiera pliki. Spowoduje to" RED " nieodwracalne" YELLOW " zmiany. Kontynowac? " GREEN "[yes/no]\n");
				scanf("%s",letsProceed);
				if(strcmp(letsProceed,"yes") == 0 || strcmp(letsProceed,"YES") == 0)
				{
					for(i = 2; i < paramCount; i++)
					{
						removeDir(PARAMETERS[i]);
					}
				}
				else
				{
					printf(YELLOW "Ocaliles %i katalogi/ow\n",paramCount - 2);
					printf(YELLOW "Jak zmienisz zdanie to zawsze mozesz tu wrocic\n");
				}
			}
			else
			{
				printf(RED "Nalezy okreslic katalog ktory chcesz usunac!\n");
			}

		}
		else
			for(i = 1; i < paramCount; i++)
			{
				if( remove(PARAMETERS[i]) == 0 )
					printf(YELLOW "Usunieto plik: %s\n",PARAMETERS[i]);
				else
					printf(RED "WYSTAPIL BLAD\n");
			}
	}
}

/* mkdir - tworzenie katalogow w formie mkdir [new_dir] [new_dir2] [...], prostych hierrarchi jednokatalogowych, kilku katalogow synow w ojcu */

void makeDirectory()
{
	char *singleKids[20];
	char *singleWords[100], *divider;
	char actualpath[1024];
	getcwd(actualpath,1024);
	int howManyDir = 0;
    int i;
    if ( paramCount == 1 )
		printf(RED "Brak nazwy katalogu. Aby stworzyc poprawnie katalog nalezy podac jego nazwe.\n");
	else if(strcmp(PARAMETERS[1],"-p") == 0)
	{
		if (paramCount < 3)
			printf(RED "Podaj sciezke ktora chcesz utworzyc!\n");
		else
		{
			int i = 0;
			int j;

			divider = strtok(PARAMETERS[2]," /");
			/* rozbija wejsciowa linie na pojedyncze slowa */
			while ( divider != NULL )
			{
				singleWords[i] = strdup( divider );
				divider = strtok(NULL," /");
				i = i + 1;
			}
			/* przypisywanie globalnej tablicy parametrow rozdzielone slowa z wejsciowej lini */
			for (j = 0; j < i; j++)
			{
				directors[j] = singleWords[j];
			}
			howManyDir = j;
			directors[i] = NULL;
			for(i = 0; i < howManyDir; i++)
			{
				mkdir(directors[i],0777);
				chdir(directors[i]);
			}
			chdir(actualpath);
		}
	}
	else if(strcmp(PARAMETERS[1],"-M") == 0)
	{
		if (paramCount < 3)
			printf(RED "Podaj strukture katalogow ktora chcesz utworzyc\n");
		else
		{
			int i = 0;
			int j;

			divider = strtok(PARAMETERS[2]," {");
			/* rozbija wejsciowa linie na pojedyncze slowa */
			while ( divider != NULL )
			{
				singleWords[i] = strdup( divider );
				divider = strtok(NULL," {");
				i = i + 1;
			}
			/* przypisywanie globalnej tablicy parametrow rozdzielone slowa z wejsciowej lini */
			for (j = 0; j < i; j++)
			{
				directors[j] = singleWords[j];
			}
			howManyDir = j;
			directors[i] = NULL;
			int lengthFirst,lengthSecond;
			lengthFirst = strlen(directors[1]);
			lengthSecond = strlen(directors[0]);
			char *chToDel = directors[1] + (lengthFirst-1);
			while(*chToDel == '}')
				*chToDel-- = '\0';
			char *del = directors[0] + (lengthSecond-1);
			while(*del == '-')
				*del-- = '\0';

			divider = strtok(directors[1]," ,");

			while ( divider != NULL )
			{
				singleWords[i] = strdup( divider );
				divider = strtok(NULL," ,");
				i = i + 1;
			}
			/* przypisywanie globalnej tablicy parametrow rozdzielone slowa z wejsciowej lini */
			for (j = 0; j < i; j++)
			{
				singleKids[j] = singleWords[j];
			}
			howManyDir = j;
			singleKids[i] = NULL;
			mkdir(directors[0],0777);
			changeDirectory(directors[0]);
			for(j = 2; j < howManyDir; j++)
			{
				mkdir(singleKids[j],0777);
				printf(YELLOW "Pomyslnie utworzono katalog %s w katalogu %s\n",singleKids[j],directors[0]);
			}
			changeDirectory(actualpath);
		}
	}
    else
    {
		for(i = 1; i < paramCount; i++)
	    {
	        if( mkdir(PARAMETERS[i],0777) == -1)
		    	printf(RED "Katalog o nazwie %s juz istnieje.\n",PARAMETERS[i]);
			else
			{
			    mkdir(PARAMETERS[i],0777);
			    printf(YELLOW "Pomyslnie stworzono katalog o nazwie %s\n",PARAMETERS[i]);
			}
	    }
     }
}

/* parsowanie tekstu */

void parsingTheLine(char line[])
{
	int i = 0;
	int j;

	char *singleWords[100], *divider;

	divider = strtok(line," \n");
	/* rozbija wejsciowa linie na pojedyncze slowa */
	while ( divider != NULL )
	{
		singleWords[i] = strdup( divider );
		divider = strtok(NULL," \n");
		i = i + 1;
	}
	/* pierwsze slowo to zawsze komenda wiec ja zapisuje tu */
	strcpy(COMMAND,singleWords[0]);
	/* przypisywanie globalnej tablicy parametrow rozdzielone slowa z wejsciowej lini */
	for (j = 0; j < i; j++)
	{
		PARAMETERS[j] = singleWords[j];
	}
	paramCount = j;
	PARAMETERS[i] = NULL;
}

/* help - wyświetla na ekranie informacje o autorze programu i oferowanych przez niego funkcjonalnościach */

void helpCommand()
{
    puts(RED "******* K.G. SHELL *******"
        "\n"BLUE"Operating Systems Project"
        "\n"RED"-- Supported commands: --"
        "\n"BLUE"> "WHITE"cd"
        "\n"BLUE"> "WHITE"help"
        "\n"BLUE"> "WHITE"exit"
        "\n"BLUE"> "YELLOW"* "RED"mkdir"YELLOW" * " WHITE "-" YELLOW " foldery, prostoliniowe foldery zagniezdzone (parametr -p),foldery z kilkoma sub-folderami(parametr -M)"
        "\n"BLUE"> "YELLOW"* "RED"cp"YELLOW" *"
        "\n"BLUE"> "YELLOW"* "RED"rm"YELLOW" * " WHITE "-" YELLOW " pliki jak i foldery (parametr -R)"
        "\n"BLUE"> "YELLOW"* "RED"wc"YELLOW" *"
        "\n"BLUE"> "GREEN"clear"
        "\n"BLUE"> "GREEN"history"
        "\n"BLUE"> "GREEN"pwd"
        "\n"BLUE"> "GREEN"login"
        "\n"BLUE"Autor: "RED"Kajetan Galeziowski " GREEN "06-SOPLI0-19Z");

}

/* obsluga polecen zaimplementowanych */

void whatToDoWithCommand(char *inputString, char *firstParam, char *secondParam, char *thirdParam)
{
    int i;
    int commandSwitcher = 0;
    for(i = 0; i < COUNTOFCOMMANDS; i++)
    {
		if( strcmp( inputString,functionalCommands[i]) == 0 )
		{
		    commandSwitcher = i;
		    break;  
		}
		else
		    commandSwitcher = 99;
    }
    
    switch(commandSwitcher)
    {
	case 0:
	    changeDirectory(firstParam);
	    break;
	case 1:
	    helpCommand();
        break;
	case 2:
	    makeDirectory();
	    break;
	case 3:
		copyingFiles(firstParam,secondParam,thirdParam);
		break;
	case 4:
	    clearTerminal();
	    break;
	case 5:
	    printingHistory();
	    break;
	case 6:
		printWorkingDirectory();
		break;
	case 7:
		printLogin();
		break;
	case 8:
		wordCount(firstParam);
		break;
	case 9:
		removeDirsOrFiles();
		break;
	case 99:
	    puts(RED "Nieznana komenda.Wpisz help aby uzyskac informacje odnosnie komend obslugiwanych przez K.G SHELL");
	    break;
    }
}

/* FUNKCJA GLOWNA */

int main()
{
	home = getenv("HOME");
	char inputLine[512] = "";
	char *toHistory;
    login = getenv("USER");
    getcwd(PATH,sizeof(PATH));
    sprintf(inputLine,BLUE "%s:" GREEN "%s: "WHITE,login,PATH);
    while ((toHistory = readline(inputLine))) 
    	{
    		trackingHistory(toHistory);
    		parsingTheLine(toHistory);
    		if( mineOrShells(COMMAND) == 1)
				whatToDoWithCommand(PARAMETERS[0],PARAMETERS[1],PARAMETERS[2],PARAMETERS[3]);
			else
			{
				if( strcmp(PARAMETERS[0],"exit") == 0)
					exit(0);
				else if (fork () != 0 )
					wait( NULL );
				else
				{	
					execvp(COMMAND,PARAMETERS);
					if(execvp(COMMAND,PARAMETERS) == -1)
					{
						/* perror("execvp"); */ 
						printf(RED "Nieznana komenda - program nie znajduje sie w katalogach opisanych zmienna srodowiskowa PATH ani nie jest zaimplementowany bezposrednio w K.G SHELL - wpisz help aby uzyskac informacje odnosnie wbudowanych funkcjonalnosci.\n");
						exit(0);
					}
				}
			}
    		add_history(toHistory);
    		free(toHistory);
    		historyCount++;
    		login = getenv("USER");
    		getcwd(PATH,sizeof(PATH));
    		sprintf(inputLine,BLUE "%s:" GREEN "%s: "WHITE,login,PATH);
	}
    return 0;
}