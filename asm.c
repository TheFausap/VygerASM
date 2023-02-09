#define MEMSIZE	0x800 /* 512W of RAM */
#define RDONLY	0
#define WRONLY	1
#define RDWR	2
#define SSEEK	0
#define ENDF	0
#define EACCESS -1

#define IND     0x80
#define TOS	0x10
#define BOS	0x40
#define ORG	S+0x44
#define IOA	0x7ac

/* instruction format (generic) */
/* (r<<8)|(ind<<7)|(reg<<4)|op  */

/* instruction format (i/o)     */
/* (r<<8)|(t<<4)|op             */
/* t is:                        */
/*       00 = console to IOA    */
/*       01 = console from IOA  */
/*       02 = disk from IOA     */
/*       03 = disk to IOA       */

/* opcode format (integer operations) */
/* opR[#|i]nnn                        */
/* where R is regno.                  */
/*       # is immediate value         */
/*       i is indirect value          */
/*       m is the value or address    */

/* OPCODES */

#define NO	0  /* No-operation     */
#define AD	1  /* Integer addition */
#define ST	2  /* Store (R->M)     */
#define MV	3  /* Move  (M->R)     */
#define PO	4  /* Pop  SK->R0      */
#define PU	5  /* Push R0->SK      */
#define SU	6  /* Integer subtract */
#define MU	7  /* Integer multiply */
#define JU	8  /* Jump             */
#define JZ	9  /* Jump if zero     */
#define JN	10 /* Jump if not zero */
#define CM	11 /* Compare          */
#define JC	12 /* Jump Compare     */
#define BO	13 /* Boolean ops      */
#define IO	14 /* I/O instructions */
#define HL	15 /* Halt the CPU     */

int fi;
int fo;
int hlt;

int S; 	/* main storage     */
int R0;	/* GPR0 = address 0 */
int R1; /* GPR1 = address 1 */
int R2; /* GPR2 = address 2 */
int R3; /* GPR3 = address 3 */
int PC; /* Program Counter  */
int sp; /* stack pointer    */
int fl; /* machine flags    */

#define ZR	0x1
#define OV	0x2
#define EQ	0x4
#define GR	0x8
#define LS	0x10

init()
{
	int i;
        int lS;

	S = malloc(MEMSIZE);
	lS = S;
	PC = ORG; /* start of program area */
	printf("%x,%x\n",S,PC);
	R0 = S;
	R1 = S+0x4;
	R2 = S+0x8;
	R3 = S+0xc;
	sp = 0x40;
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

	if ((op == AD)  || (op == ST) || (op == MV) || (op == JU) 
			|| (op == SU) || (op == MU) || (op == JZ)
		        || (op == JN))	{
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
	} else if (op == IO) {
		/* I/O instruction use a different format */
	} else if ((op == HL) || (op == PU) || (op == PO)) {
		*(int*)PC = op;
		printf("%x@%x\n",*(int*)PC,PC);
		PC=PC+4;
	}	
}

pop()
{
	int lS;
	sp = sp + 4;
	if (sp > BOS) {
		printf("S!O\n");
		exit(16);
	}
	lS = S + sp;
	return *(int*)lS;
}

push(v)
{
	int lS;
 	lS = S + sp;
	*(int*)lS = v;
	sp = sp - 4;
	printf("PSH: %X,%X\n",lS,v);
	if (sp < TOS) {
		printf("S!U\n");
		exit(15);
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
	else {
		*(int*)lS = v;
	}
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
	int jp;
	PC = ORG;
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
		val = mbr >> 8;
		jp = 0;
		printf("%X(PC),%X\n",PC,op);
		if (op == AD) {
			if (ind)
				val = rdmem(reg,0) + rdmem(val,1);
			else
				val = rdmem(reg,0) + val;
			if (val==0)
				fl = fl | ZR;
			wrRegI(reg, val, 0);
		} else if (op == SU) {
			if (ind)
                                val = rdmem(reg,0) - rdmem(val,1);
                        else
                                val = rdmem(reg,0) - val;
                        if (val==0)
                                fl = fl | ZR;
                        wrRegI(reg, val, 0);
		} else if (op == MU) {
			if (ind)
                                val = rdmem(reg,0) * rdmem(val,1);
                        else
                                val = rdmem(reg,0) * val;
                        if (val==0)
                                fl = fl | ZR;
                        wrRegI(reg, val, 0);
		} else if (op == PU) {
			push(*(int*)S);
		} else if (op == PO) {
			*(int*)S = pop();
		} else if (op == ST) {
			if (val % 4) 
				wrMemR(val,reg, ind);
			else {
				val = val << 2;
				wrMemR(val,reg, ind);
			}
		} else if (op == MV) {
			wrRegI(reg, val, ind);
		} else if (op == JU) {
			val = val << 2;
			jp = 1;
			printf("JU:%X,%X(PC)\n",val,PC);
			printf("Jumping...\n");
			PC = val;
		} else if (op == JZ) {
			if (fl & ZR) {
				val = PC + (val << 2);
                        	jp = 1;
				printf("JZ:%X,%X(PC)\n",val,PC);
				printf("Jumping...\n");
				PC = val;
			}
		} else if (op == JN) {
			if (!(fl & ZR)) {
				val = PC + (val << 2);
                                jp = 1;
				printf("JN:%X,%X(PC)\n",val,PC);
				printf("Jumping...\n");
				PC = val;
                        }
		} else if (op == CM) {
			fl = fl & 0x3; /* reset flags */
			if (rdmem(reg,0) == val) 
				fl = fl | EQ;
			else if (rdmem(reg,0) > val)
				fl = fl | GR;
			else
				fl = fl | LS;
		} else if (op == NO) {
			/* NOP, do nothing */
		} else if (op == HL) {
			hlt = 1;
		}		
		if (jp==0) 
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
			else if (c == 'U')
				emit(SU,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		} else if (c == 'P') {
			read(fi,&c,1);
			if (c == 'U')
				emit(PU,fi);
			else if (c == 'O')
				emit(PO,fi);
			else {
				printf("U!O\n");
				exit(11);
			}
		} else if (c == 'J') {
			read(fi,&c,1);
			if (c == 'U')
				emit(JU,fi);
			else if (c == 'Z')
				emit(JZ,fi);
			else if (c == 'N')
				emit(JN,fi);
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
			else if (c == 'U')
				emit(MU,fi);
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

	for(i=0;i<MEMSIZE/4;i++) {
		printf("%04o[0x%08X]=0x%08X\n",i,lS,*(int*)lS);
		lS=lS+4;
	}
	return 0;
}

main(argc,argv)
{
	init();
	load();
	printf("Program loaded.\n");
	eval();
	dump();
	close(fi);
	return 0;
}
