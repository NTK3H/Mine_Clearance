#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>

typedef struct _Console_Mouse_Click_Event
{
	unsigned int Button:2;
	unsigned int DoubleClick:1;
	COORD cPos;
} MCE;

int Mouse_ReadConClickEvent(MCE *m)
{
	INPUT_RECORD inRec;
	DWORD nRead;
	ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &inRec, 1, &nRead);
	if(inRec.EventType != MOUSE_EVENT)
		return FALSE;
	m->cPos = inRec.Event.MouseEvent.dwMousePosition;
	if(inRec.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)
		m->DoubleClick = 1;
	else
		m->DoubleClick = 0;
	if(inRec.Event.MouseEvent.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
		m->Button = 0;
	else if(inRec.Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED)
		m->Button = 1;
	else if(inRec.Event.MouseEvent.dwButtonState == FROM_LEFT_2ND_BUTTON_PRESSED)
		m->Button = 2;
	else
		m->Button = 3;
	return TRUE;
}

void gotoxy(int x, int y)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hOut,(COORD){x,y});
}

void clrscr()
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	GetConsoleScreenBufferInfo(hOut,&csbiInfo);
	DWORD dummy;
	COORD Home = {0,0};
	FillConsoleOutputCharacter(hOut,' ',csbiInfo.dwSize.X * csbiInfo.dwSize.Y,Home,&dummy);
	SetConsoleCursorPosition(hOut,Home);
}

int SetConSize(int iWeight, int iHeight)
{
	int ret = 0;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD size = {iWeight, iHeight};
	ret += SetConsoleScreenBufferSize(hOut,size);
	SMALL_RECT rc = {0,0,iWeight-1,iHeight-1};
	ret += SetConsoleWindowInfo(hOut , TRUE, &rc);
	ret += SetConsoleScreenBufferSize(hOut,size);
	return ret;
}

int isTouch(int **map, COORD *p, COORD *size)
{
	if(p->X<0 || p->Y<0 || p->X>size->X-1 || p->Y>size->Y-1)
		return 0;
	return map[p->X/2][p->Y];
}

void st(int t)
{
	HWND hwnd = GetConsoleWindow();
	LONG lWindowStyle = GetWindowLong(hwnd, GWL_EXSTYLE) |WS_EX_LAYERED;
	SetWindowLong(hwnd, GWL_EXSTYLE, lWindowStyle);
	SetLayeredWindowAttributes(hwnd, 0, t, LWA_ALPHA);
}

void init_window()
{
	st(255);
	CONSOLE_CURSOR_INFO cursor_info = {1, 0}; 
	SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cursor_info);
	SetConsoleTitle("扫雷 - 主界面");
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hIn, &mode);
	mode &= ~ENABLE_QUICK_EDIT_MODE;
	SetConsoleMode(hIn, mode);
}

int Clear_Nums = 0;

void clear(int **mi, int **op, COORD *p, COORD *size)
{
	if(p->X<0 || p->Y<0 || p->X>size->X-1 || p->Y>size->Y-1)
		return;
	gotoxy(p->X,p->Y);
	int n = 0;
	for(int i=0;i<3;++i)
		for(int j=0;j<3;++j)
			n+=isTouch(mi, &(COORD){p->X-2+i*2,p->Y-1+j}, size);
	char nom[3];
	strcpy(nom,"①");
	nom[1] = nom[1]+n-1;
	printf(n?"%s":" ",nom);
	op[p->X/2][p->Y] = 1;
	if(n==0)
		for(int i=0;i<3;++i)
			for(int j=0;j<3;++j)
			{
				COORD new = {p->X-2+i*2,p->Y-1+j};
				if(new.X<0 || new.Y<0 || new.X>size->X-1 || new.Y>size->Y-1)
					continue;
				if(!op[new.X/2][new.Y])
					clear(mi, op, &new, size);
			}
	++Clear_Nums;
}

int gamemain(int x, int y, int n)
{
	if(n>x*y-14)
		return -1;
	clrscr();
	Clear_Nums = 0;
	srand((unsigned)time(NULL));
	int Mine_Nums = n;
	COORD size = {x,y+1};
	if(!SetConSize(size.X*2,size.Y))
		return -2;
	int **mi = (int**)malloc(sizeof(int*)*x);
	for(int i=0;i<x;++i)
		mi[i] = (int*)calloc(1,sizeof(int)*y);
	int **op = (int**)malloc(sizeof(int*)*x);
	for(int i=0;i<x;++i)
		op[i] = (int*)calloc(1,sizeof(int)*y);
	COORD tmp;
	for(int i=0;i<Mine_Nums;++i)
	{
		tmp.X = rand()%x;
		tmp.Y = rand()%y;
		while(mi[tmp.X][tmp.Y])
		{
			tmp.X = rand()%x;
			tmp.Y = rand()%y;
		}
		mi[tmp.X][tmp.Y] = 1;
	}
	for(int i=0;i<y;++i)
		for(int j=0;j<x*2;j+=2)
		{
			gotoxy(j,i);
			printf("■");
		}
	gotoxy(0,size.Y);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
	printf("按Esc键返回菜单");
	MCE *m = (MCE*)malloc(sizeof(MCE));
	Mouse_ReadConClickEvent(m);
	int first = 0;
	clock_t start = clock();
	clock_t rt = 0;
	int pmn = n;
	char p[50];
	sprintf(p,"扫雷 - 游戏中 | 剩余雷数:%4d",pmn);
	SetConsoleTitle(p);
	for(;;)
	{
		SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_INTENSITY|FOREGROUND_RED|FOREGROUND_GREEN);
		if(GetKeyState(VK_ESCAPE)<0)
			return 0;
		if(!Mouse_ReadConClickEvent(m) || m->cPos.X<0 || m->cPos.Y<0 || m->cPos.X>x*2-1 || m->cPos.Y>y-1)
			continue;
		if(m->Button == 0)
		{
			m->cPos.X = m->cPos.X&1?m->cPos.X-1:m->cPos.X;
			gotoxy(m->cPos.X,m->cPos.Y);
			if((op[m->cPos.X/2][m->cPos.Y] == 0) || (op[m->cPos.X/2][m->cPos.Y] == 3))
			{
				op[m->cPos.X/2][m->cPos.Y] = 1;
				if(isTouch(mi, &m->cPos, &(COORD){x*2,y}))
				{
					if(!first)
					{
						mi[m->cPos.X/2][m->cPos.Y] = 0;
						op[m->cPos.X/2][m->cPos.Y] = 0;
						tmp.X = rand()%x;
						tmp.Y = rand()%y;
						while(((tmp.X==m->cPos.X/2) && (tmp.Y==m->cPos.Y)) || mi[tmp.X][tmp.Y])
						{
							tmp.X = rand()%x;
							tmp.Y = rand()%y;
						}
						mi[tmp.X][tmp.Y] = first = 1;
						clear(mi,op,&m->cPos, &(COORD){x*2,y});
						first = 1;
						continue;
					}
lose:
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED | FOREGROUND_BLUE);
					for(int i=0;i<y;++i)
						for(int j=0;j<x;++j)
							if(mi[j][i])
							{
								gotoxy(j*2,i);
								printf("●");
							}
					gotoxy(m->cPos.X,m->cPos.Y);
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED);
					printf("●");
					MessageBox(NULL, "你输了!", "游戏失败", MB_OK|MB_ICONWARNING);
					for(int i=0;i<x;++i)
						free(mi[i]);
					free(mi);
					for(int i=0;i<x;++i)
						free(op[i]);
					free(op);
					return 0;
				}
				else
				{
					clock_t rts = clock();
					clear(mi,op,&m->cPos, &(COORD){x*2,y});
					clock_t rte = clock();
					rt += rte-rts;
				}
			}
			first = 1;
			while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
		}
		else if(m->Button == 1)
		{
			m->cPos.X = m->cPos.X&1?m->cPos.X-1:m->cPos.X;
			gotoxy(m->cPos.X,m->cPos.Y);
			switch(op[m->cPos.X/2][m->cPos.Y])
			{
				case 0:
					--pmn;
					op[m->cPos.X/2][m->cPos.Y] = 2;
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
					printf("★");
					break;
				case 2:
					++pmn;
					op[m->cPos.X/2][m->cPos.Y] = 3;
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf("？");
					break;
				case 3:
					op[m->cPos.X/2][m->cPos.Y] = 0;
					SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
					printf("■");
					break;
			}
			sprintf(p,"扫雷 - 游戏中 | 剩余雷数:%4d",pmn);
			SetConsoleTitle(p);
			while(!Mouse_ReadConClickEvent(m) || m->Button == 1);
		}
		else if(m->Button == 2)
		{
			m->cPos.X = m->cPos.X&1?m->cPos.X-1:m->cPos.X;
			if(op[m->cPos.X/2][m->cPos.Y] != 1)
				continue;
			gotoxy(m->cPos.X,m->cPos.Y);
			int n = 0, s = 0;
			for(int i=0;i<3;++i)
				for(int j=0;j<3;++j)
				{
					n+=isTouch(mi, &(COORD){m->cPos.X-2+i*2,m->cPos.Y-1+j}, &(COORD){x*2,y});
					if(isTouch(op, &(COORD){m->cPos.X-2+i*2,m->cPos.Y-1+j}, &(COORD){x*2,y}) == 2)
						++s;
				}
			if(n == s)
				for(int i=0;i<3;++i)
					for(int j=0;j<3;++j)
					{
						int isT = isTouch(op, &(COORD){m->cPos.X-2+i*2,m->cPos.Y-1+j}, &(COORD){x*2,y});
						if(m->cPos.X>=0 && m->cPos.Y>=0 && m->cPos.X<=x*2-1 && m->cPos.Y<=y-1 
								&&(isT == 0 || isT == 3))
						{
							clock_t rts = clock();
							clear(mi,op,&(COORD){m->cPos.X-2+i*2,m->cPos.Y-1+j}, &(COORD){x*2,y});
							clock_t rte = clock();
							rt += rte-rts;
							if(isTouch(mi, &(COORD){m->cPos.X-2+i*2,m->cPos.Y-1+j}, &(COORD){x*2,y}))
							{
								m->cPos.X = m->cPos.X-2+i*2;
								m->cPos.Y = m->cPos.Y-1+j;
								goto lose;
							}
						}

					}
			while(!Mouse_ReadConClickEvent(m) || m->Button == 2);
		}
		if(Clear_Nums == x*y-Mine_Nums)
		{
			clock_t end = clock();
			char p[30];
			sprintf(p,"你赢了!\n用时: %.3f秒",(double)(end-start-rt)/1000);
			MessageBox(NULL, p, "游戏胜利", MB_OK|MB_ICONINFORMATION);
			for(int i=0;i<x;++i)
				free(mi[i]);
			free(mi);
			for(int i=0;i<x;++i)
				free(op[i]);
			free(op);
			return 0;
		}
	}
}

#define BASE_LINE_X 35
#define BASE_LINE_Y 3

void printmain()
{
	clrscr();
	SetConsoleTitle("扫雷 - 主界面");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
	SetConSize(90,30);
	gotoxy(BASE_LINE_X+5,BASE_LINE_Y);
	printf("扫雷");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+5);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+6);
	printf("|   开始游戏   |");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+7);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+9);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+10);
	printf("|     设置     |");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+11);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+13);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+14);
	printf("|     帮助     |");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+15);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+17);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+18);
	printf("|     关于     |");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+19);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+21);
	printf("*==============*");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+22);
	printf("|     退出     |");
	gotoxy(BASE_LINE_X,BASE_LINE_Y+23);
	printf("*==============*");
}

void help()
{
	clrscr();
	SetConsoleTitle("扫雷 - 帮助");
	SetConSize(90,30);
	gotoxy(10,3);
	printf("帮助: ");
	gotoxy(10,BASE_LINE_Y+3);
	printf("游戏目标: 找出空方块并避免触雷");
	gotoxy(10,BASE_LINE_Y+5);
	printf("玩法: ");
	gotoxy(10,BASE_LINE_Y+6);
	printf("* 挖开地雷,游戏宣告结束");
	gotoxy(10,BASE_LINE_Y+7);
	printf("* 挖开空方块,可以继续玩");
	gotoxy(10,BASE_LINE_Y+8);
	printf("* 挖开数字,则表示在其周围的八个方块中共有多少雷,可以使用该信息");
	gotoxy(10,BASE_LINE_Y+9);
	printf("  推断出能够安全单击附近的哪些方块");
	gotoxy(10,BASE_LINE_Y+11);
	printf("设置选项说明: ");
	gotoxy(10,BASE_LINE_Y+12);
	printf("* 宽度 - 方块的X轴个数");
	gotoxy(10,BASE_LINE_Y+13);
	printf("* 高度 - 方块的Y轴个数");
	gotoxy(10,BASE_LINE_Y+14);
	printf("* 雷数 - 雷的数量(大致的数量)");
	gotoxy(10,BASE_LINE_Y+15);
	printf("* 雷数细准较 - 当需要精确的雷数时调整此选项即可");
	gotoxy(10,BASE_LINE_Y+16);
	printf("* 界面透明度 - 更改界面的透明度(部分显卡可能不支持此选项)");

	gotoxy(60,21);
	printf("*==============*");
	gotoxy(60,22);
	printf("|     确定     |");
	gotoxy(60,23);
	printf("*==============*");
	MCE *m = (MCE*)malloc(sizeof(MCE));
	while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
	while(!Mouse_ReadConClickEvent(m) || m->Button || !(m->cPos.Y >= 21 && m->cPos.Y <= 23 && m->cPos.X>59 && m->cPos.X<76));
	free(m);
}

void about()
{
	clrscr();
	SetConsoleTitle("扫雷 - 关于");
	SetConSize(90,30);
	gotoxy(10,3);
	printf("关于: ");
	gotoxy(10,BASE_LINE_Y+5);
	printf("By: Truth");
	gotoxy(10,BASE_LINE_Y+7);
	printf("E-Mail: 2568878009@qq.com");
	gotoxy(10,BASE_LINE_Y+9);
	printf("日期: 2018/9/18");
	gotoxy(60,21);
	printf("*==============*");
	gotoxy(60,22);
	printf("|     确定     |");
	gotoxy(60,23);
	printf("*==============*");
	MCE *m = (MCE*)malloc(sizeof(MCE));
	while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
	while(!Mouse_ReadConClickEvent(m) || m->Button || !(m->cPos.Y >= 21 && m->cPos.Y <= 23 && m->cPos.X>59 && m->cPos.X<76));
	free(m);
}

typedef struct _arguments_of_gamemain
{
	int x;
	int y;
	int n;
	int sn;
	int t;
} ag;

void printsettings(ag *a, ag *sr)
{
	gotoxy(10,3);
	printf("设置: ");
	gotoxy(10,BASE_LINE_Y+5);
	printf("宽度:      [");
	printf("============================================================");
	printf("] %3d",a->x);
	gotoxy(22+sr->x,BASE_LINE_Y+5);
	printf("#");
	gotoxy(10,BASE_LINE_Y+7);
	printf("高度:      [");
	printf("============================================================");
	printf("] %3d",a->y);
	gotoxy(22+sr->y,BASE_LINE_Y+7);
	printf("#");
	gotoxy(10,BASE_LINE_Y+9);
	printf("雷数:      [");
	printf("============================================================");
	printf("]%4d",a->n);
	gotoxy(22+sr->n,BASE_LINE_Y+9);
	printf("#");
	gotoxy(10,BASE_LINE_Y+11);
	printf("雷数细准较:[");
	printf("=============================^==============================");
	printf("] %+3d",a->sn);
	gotoxy(51+sr->sn,BASE_LINE_Y+11);
	printf("#");
	gotoxy(10,BASE_LINE_Y+13);
	printf("界面透明度:[");
	printf("============================================================");
	printf("] %3d",a->t);
	gotoxy(22+sr->t,BASE_LINE_Y+13);
	printf("#");
}

void settings(ag *a, ag *sr)
{
	clrscr();
	SetConsoleTitle("扫雷 - 设置");
	printsettings(a,sr);
	gotoxy(17,21);
	printf("*==============*");
	gotoxy(17,22);
	printf("|     确定     |");
	gotoxy(17,23);
	printf("*==============*");
	gotoxy(57,21);
	printf("*==============*");
	gotoxy(57,22);
	printf("|    默认值    |");
	gotoxy(57,23);
	printf("*==============*");
	MCE *m = (MCE*)malloc(sizeof(MCE));
	while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
	ag *copy = (ag*)malloc(sizeof(ag));
	for(;;)
	{
		if(!Mouse_ReadConClickEvent(m))
			continue;
		if(m->Button == 0)
		{
			if(m->cPos.Y >= 21 && m->cPos.Y <= 23 && m->cPos.X>16 && m->cPos.X<33)
			{
				free(m);
				free(copy);
				return;
			}
			else if(m->cPos.Y >= 21 && m->cPos.Y <= 23 && m->cPos.X>56 && m->cPos.X<73)
			{
				a->x=9;
				a->y=9;
				a->n=10;
				a->sn=0;
				a->t=255;

				sr->x = 0;
				sr->y = 0;
				sr->n = 0;
				sr->sn = 0;
				sr->t = 59;
				printsettings(a,sr);
				st(a->t);
				while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			else if(m->cPos.Y == BASE_LINE_Y+5 && m->cPos.X>21 && m->cPos.X<82)
			{
				do{
					if(m->cPos.X<=21 || m->cPos.X>=82 || m->cPos.X-22 == sr->x)
						continue;
					gotoxy(sr->x+22,BASE_LINE_Y+5);
					printf("=");
					sr->x = m->cPos.X-22;
					a->x = (double)(m->cPos.X-22)/59.0*70+9;
					gotoxy(m->cPos.X,BASE_LINE_Y+5);
					printf("#");
					gotoxy(84,BASE_LINE_Y+5);
					printf("%3d",a->x);

					int nx = ((double)a->n-10)/(double)(a->x*a->y-24)*59.0;
					nx = (nx!=0) && (nx!=59)?nx+1:nx;
					if(sr->n != nx)
					{
						gotoxy(sr->n+22,BASE_LINE_Y+9);
						printf("=");
						sr->n = nx;
						if(nx>59)
						{
							a->n = a->x*a->y-14;
							gotoxy(83,BASE_LINE_Y+9);
							printf("%4d",a->n);
							sr->n = nx=59;
						}
						gotoxy(nx+22,BASE_LINE_Y+9);
						printf("#");
					}

					if(sr->sn != 0)
					{
						gotoxy(sr->sn+51,BASE_LINE_Y+11);
						printf("=");
						a->sn=0;
						sr->sn=0;
						gotoxy(51,BASE_LINE_Y+11);
						printf("#");
						gotoxy(82,BASE_LINE_Y+11);
						printf("] %+3d",a->sn);
					}
				} while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			else if(m->cPos.Y == BASE_LINE_Y+7 && m->cPos.X>21 && m->cPos.X<82)
			{
				do{
					if(m->cPos.X<=21 || m->cPos.X>=82 || m->cPos.X-22 == sr->y)
						continue;
					gotoxy(sr->y+22,BASE_LINE_Y+7);
					printf("=");
					sr->y = m->cPos.X-22;
					a->y = (double)(m->cPos.X-22)/59.0*41+9;
					gotoxy(m->cPos.X,BASE_LINE_Y+7);
					printf("#");
					gotoxy(84,BASE_LINE_Y+7);
					printf("%3d",a->y);

					int nx = ((double)a->n-10)/(double)(a->x*a->y-24)*59.0;
					nx = (nx!=0) && (nx!=59)?nx+1:nx;
					if(sr->n != nx)
					{
						gotoxy(sr->n+22,BASE_LINE_Y+9);
						printf("=");
						sr->n = nx;
						if(nx>59)
						{
							a->n = a->x*a->y-14;
							gotoxy(83,BASE_LINE_Y+9);
							printf("%4d",a->n);
							sr->n = nx=59;
						}
						gotoxy(nx+22,BASE_LINE_Y+9);
						printf("#");
					}
					if(sr->sn != 0)
					{
						gotoxy(sr->sn+51,BASE_LINE_Y+11);
						printf("=");
						a->sn=0;
						sr->sn=0;
						gotoxy(51,BASE_LINE_Y+11);
						printf("#");
						gotoxy(82,BASE_LINE_Y+11);
						printf("] %+3d",a->sn);
					}
				} while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			else if(m->cPos.Y == BASE_LINE_Y+9 && m->cPos.X>21 && m->cPos.X<82)
			{
				do{
					if(m->cPos.X<=21 || m->cPos.X>=82 || m->cPos.X-22 == sr->n)
						continue;
					gotoxy(sr->n+22,BASE_LINE_Y+9);
					printf("=");
					sr->n = m->cPos.X-22;
					a->n = (double)(m->cPos.X-22)/59.0*(a->x*a->y-24)+10;
					gotoxy(m->cPos.X,BASE_LINE_Y+9);
					printf("#");
					gotoxy(83,BASE_LINE_Y+9);
					printf("%4d",a->n);

					if(sr->sn != 0)
					{
						gotoxy(sr->sn+51,BASE_LINE_Y+11);
						printf("=");
						a->sn=0;
						sr->sn=0;
						gotoxy(51,BASE_LINE_Y+11);
						printf("#");
						gotoxy(82,BASE_LINE_Y+11);
						printf("] %+3d",a->sn);
					}
				} while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			else if(m->cPos.Y == BASE_LINE_Y+11 && m->cPos.X>21 && m->cPos.X<82)
			{
				do{
					if(m->cPos.X<=21 || m->cPos.X>=82 || m->cPos.X-51 == sr->sn)
						continue;
					a->n -= a->sn;
					gotoxy(sr->sn+51,BASE_LINE_Y+11);
					printf(sr->sn==0?"^":"=");
					sr->sn = m->cPos.X-51;
					a->sn = (double)(m->cPos.X-22) / 59.0 * (double)(a->x*a->y/59.0) - (a->x*a->y/59.0)/2;
					gotoxy(m->cPos.X,BASE_LINE_Y+11);
					printf("#");
					gotoxy(84,BASE_LINE_Y+11);
					printf("%+3d",a->sn);

					a->n+=a->sn;
					a->n = a->n<=0?1:a->n;
					int ntmp = sr->n;
					sr->n = ((double)a->n-10)/(double)(a->x*a->y-24)*59.0;
					sr->n += (sr->n!=0) && (sr->n!=59)?1:0;
					if(ntmp != sr->n)
					{
						gotoxy(ntmp+22, BASE_LINE_Y+9);
						printf("=");
						gotoxy(sr->n+22, BASE_LINE_Y+9);
						printf("#");
					}
					gotoxy(83,BASE_LINE_Y+9);
					printf("%4d",a->n);
				} while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			else if(m->cPos.Y == BASE_LINE_Y+13 && m->cPos.X>21 && m->cPos.X<82)
			{
				do{
					if(m->cPos.X<=21 || m->cPos.X>=82 || m->cPos.X-22 == sr->t)
						continue;
					gotoxy(sr->t+22,BASE_LINE_Y+13);
					printf("=");
					sr->t = m->cPos.X-22;
					a->t = (double)(m->cPos.X-22)/59.0*155+100;
					gotoxy(m->cPos.X,BASE_LINE_Y+13);
					printf("#");
					gotoxy(84,BASE_LINE_Y+13);
					st(a->t);
					printf("%3d",a->t);
				} while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
		}
	}
}

int main(int argc, char *argv[])
{
	init_window();
	ag *a = (ag*)malloc(sizeof(ag));
	a->x = 9;
	a->y = 9;
	a->n = 10;
	a->sn = 0;
	a->t = 255;
	ag *sr = (ag*)malloc(sizeof(ag));
	sr->x = 0;
	sr->y = 0;
	sr->n = 0;
	sr->sn = 0;
	sr->t = 59;
	printmain();
	MCE *m = (MCE*)malloc(sizeof(MCE));
	for(;;)
	{
		if(!Mouse_ReadConClickEvent(m) || m->Button)
			continue;
		if(m->cPos.Y >= BASE_LINE_Y+5 && m->cPos.Y <= BASE_LINE_Y+7 && m->cPos.X>=BASE_LINE_X && m->cPos.X<BASE_LINE_X+16)
		{
			int err = gamemain(a->x, a->y, a->n);
			if(err != 0)
			{
				SetConSize(90,30);
				clrscr();
				switch(err)
				{
					case -1:
						gotoxy(29,7);
						printf("雷数设置过大,请重新设置雷数!");
						break;
					case -2:
						gotoxy(27,7);
						printf("宽度和高度差距过大,请重新设置宽高!");
						break;
					default:
						printf("设置错误,原因未知");
						break;
				}
				gotoxy(35,17);
				printf("*==============*");
				gotoxy(35,18);
				printf("|     确定     |");
				gotoxy(35,19);
				printf("*==============*");
				while(!Mouse_ReadConClickEvent(m) || m->Button || !(m->cPos.Y >= 17 && m->cPos.Y <= 19 && m->cPos.X>34 && m->cPos.X<51));
				a->n -= a->sn;
				a->sn = 0;
				sr->sn = 0;
				settings(a,sr);
				while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
			}
			Mouse_ReadConClickEvent(m);
			printmain();
			continue;
		}
		if(m->cPos.Y >= BASE_LINE_Y+9 && m->cPos.Y <= BASE_LINE_Y+11 && m->cPos.X>=BASE_LINE_X && m->cPos.X<BASE_LINE_X+16)
		{
			settings(a,sr);
			printmain();
			while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
		}
		if(m->cPos.Y >= BASE_LINE_Y+13 && m->cPos.Y <= BASE_LINE_Y+15 && m->cPos.X>=BASE_LINE_X && m->cPos.X<BASE_LINE_X+16)
		{
			help();
			printmain();
			while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
		}
		if(m->cPos.Y >= BASE_LINE_Y+17 && m->cPos.Y <= BASE_LINE_Y+19 && m->cPos.X>=BASE_LINE_X && m->cPos.X<BASE_LINE_X+16)
		{
			about();
			printmain();
			while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
		}
		if(m->cPos.Y >= BASE_LINE_Y+21 && m->cPos.Y <= BASE_LINE_Y+23 && m->cPos.X>=BASE_LINE_X && m->cPos.X<BASE_LINE_X+16)
		{
			for(int i=a->t;i>=0;--i)
			{
				st(i);
				Sleep(2);
			}
			return 0;
		}
		while(!Mouse_ReadConClickEvent(m) || m->Button == 0);
	}
}
