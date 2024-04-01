#include <stdio.h>
#include <ctype.h>
#include <string.h>

main()
{
	static char name1[]="ThisIsATestOfTheEmergencyBroadcastSystem.";
	char name2[100];
	int i;
	int j;

	name2[0]=name1[0];
	i=j=1;
	while(name1[i]){
		if(isupper(name1[i])){
			name2[j++]=' ';
			name2[j]=tolower(name1[i]);
		}else name2[j]=name1[i];
		j++;
		i++;
	}
	name2[j]='\0';
	printf("name1=%s\n",name1);
	printf("name2=%s\n",name2);
}