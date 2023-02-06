#define MEMSIZE	0x400 /* 1KB of RAM */
#define RDONLY	0
#define WRONLY	1
#define RDWR	2
#define SSEEK	0
#define ENDF	0
#define EACCESS -1

#define IND     0x80

/* instruction format           */
/* (r<<8)|(ind<<7)|(reg<<4)|op) */

/* opcode format (integer operations) */
/* opR[#|i]nnn                        */
/* where R is regno.                  */
/*       # is immediate value         */
/*       i is indirect value          */
/*       m is the value or address    */

/* OPCODES */

#define NO	0
#define AD	1
#define ST	2
#define MV	3
#define HL	15

int fi;
int fo;
int hlt;

int S; 	/* main storage */
int R0;	/* GPR0 = address 0 */
int R1; /* GPR1 = address 1 */
int R2; /* GPR2 = address 2 */
int R3; /* GPR3 = address 3 */
int PC; /* Program Counter */

init()
{
	int i;
        int lS;

	S = malloc(MEMSIZE);
	lS = S;
	PC = S+0x10; /* start of program area */
	printf("%x,%x\n",S,PC);
	R0 = S;
	R1 = S+0x4;
	R2 = S+0x8;
	R3 = S+0xc;
	hlt = 0;
	for(i=0;i<MEMSIZE/4;i++) {
		*(int*)lS = 0;
		lS=lS+4;
	}
	return 0;
}

getnum(f)
{
	int c;
	int s;
	int s1;

	s = malloc(10);
	s1 = s;
	c = 0;

	read(f,&c,1);
	while(c != 10) {
		*(char *)s1 = c;
		s1++;
		read(f,&c,1);
	}
	*(char*)s1 = 0;
	return atoi(s);
}

emit(op,ffi)
{
	int c;
	int r;
	int reg;
	int lS;

	c = 0;
	r = 0;
	reg = 0;
	printf("%x,%x\n",S,PC);

	if ((op == AD) || (op == ST) || (op == MV)) {
		read(ffi,&c,1);
		reg = c - 48;
		read(ffi,&c,1);
		if (c == '#') {
		       	r = getnum(ffi);
			*(int*)PC = (r<<8)|(reg<<4)|op;
			printf("%x@%x\n",*(int*)PC,PC);
			PC=PC+4;
		} else if (c == 'i') {
			r = getnum(ffi);
			*(int*)PC = (r<<8)|IND|(reg<<4)|op;
			printf("%x@%x\n",*(int*)PC,PC);
			PC=PC+4;
		} else	
			printf("W!O\n");	
	} else if (op == HL) {
		*(int*)PC = op;
		PC=PC+4;
	}	
}

wrRegI(r,v,d)
{
	int lS;
	lS = S + r;
	if (d) {
		if (v % 4)
			v = v << 2;
		*(int*)lS = rdmem(v,0);
	}
	else
		*(int*)lS = v;
}

wrMemR(a,r,d)
{
	int lS;
	if (d)
		lS = S + (rdmem(a,0) << 2);
	else
		lS = S + a;
	*(int*)lS = rdmem(r,0);
}

rdmem(a,d)
{
	int lS;
	lS = S + a;
	if (d)
		return rdmem(*(int*)lS,0);
	return *(int*)lS;
}

eval()
{
	int op;
	int reg;
	int val;
	int mbr;
	int ind;
	PC = S+0x10;
	op = 0;
	reg = 0;
	val = 0;
	mbr = 0;
	ind = 0;

	while (hlt != 1) {
		mbr = *(int*)PC;
	        op = mbr & 0xf;
	        reg = mbr & 0x30;
		ind = mbr & 0x80;
		ind = ind >> 7;
		reg = reg >> 4;
		reg = reg << 2; /* memory alignment */
		val = mbr & 0xffffff0000;
		val = val >> 8;
		if (op == AD) {
			if (ind)
				val = rdmem(reg,0) + rdmem(val,1);
			else
				val = rdmem(reg,0) + val;
			wrRegI(reg, val, 0);
		} else if (op == ST) {
			if (val % 4) {
				wrMemR(val,reg, ind);
			} else {
				val = val << 2;
				wrMemR(val,reg, ind);
			}
		} else if (op == MV) {
			wrRegI(reg, val, ind);
		} else if (op == NO) {
			/* NOP, do nothing */
		} else if (op == HL) {
			hlt = 1;
		}		
		PC = PC + 4;
	}
}
	

load()
{
	int r;
	int c;

	c = 0;

	fi = open("test.asm",RDONLY,0);
	if (fi == EACCESS) {
		printf("F!I\n");
		exit(10);
	}

	r = read(fi,&c,1);
	while (r != ENDF) {
		if (c == 'A') {
			read(fi,&c,1);
			if (c == 'D')
				emit(AD,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		} else if (c == 'S') {
			read(fi,&c,1);
			if (c == 'T')
				emit(ST,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		} else if (c == 'H') {
			read(fi,&c,1);
			if (c == 'L')
				emit(HL,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		} else if (c == 'M') {
			read(fi,&c,1);
			if (c == 'V')
				emit(MV,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		}
		r = read(fi,&c,1);
	}
	return 0;
}

dump()
{
	int i;
	int lS;

	lS = S;
	printf("%x,%x,%x\n",S,PC,lS);

	for(i=0;i<MEMSIZE/4;i++) {
		printf("%d->%x,%x\n",i,lS,*(int*)lS);
		lS=lS+4;
	}
	return 0;
}

main(argc,argv)
{
	init();
	load();
	eval();
	dump();
	close(fi);
	return 0;
}
