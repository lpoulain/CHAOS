#include "libc.h"
#include "heap.h"
#include "kernel.h"
#include "isr.h"
#include "display.h"

#define FLOPPY_IRQ 6

void flpydsk_install (int irq);

//! set current working drive
void flpydsk_set_working_drive (u8int drive);

//! get current working drive
u8int flpydsk_get_working_drive ();

//! read a sector
u8int* flpydsk_read_sector (int sectorLBA);

//! converts an LBA address to CHS
void flpydsk_lba_to_chs (int lba,int *head,int *track,int *sector);

static volatile uint FloppyIRQFlag = 0;

static void floppy_IRQ_handler(registers_t regs) {
	asm __volatile__ ("cli");
	FloppyIRQFlag = 1;
	debug_i("Ha ", 0);
	asm __volatile__ ("sti");
}

void floppy_poll() {
	while (FloppyIRQFlag == 0);
	FloppyIRQFlag = 0;
}


enum FLPYDSK_IO {

	FLPYDSK_DOR		=	0x3f2,
	FLPYDSK_MSR		=	0x3f4,
	FLPYDSK_FIFO	=	0x3f5,
	FLPYDSK_CTRL	=	0x3f7
};


enum FLPYDSK_CMD {

	FDC_CMD_READ_TRACK	=	2,
	FDC_CMD_SPECIFY		=	3,
	FDC_CMD_CHECK_STAT	=	4,
	FDC_CMD_WRITE_SECT	=	5,
	FDC_CMD_READ_SECT	=	6,
	FDC_CMD_CALIBRATE	=	7,
	FDC_CMD_CHECK_INT	=	8,
	FDC_CMD_FORMAT_TRACK=	0xd,
	FDC_CMD_SEEK		=	0xf
};


enum FLPYDSK_CMD_EXT {

	FDC_CMD_EXT_SKIP		=	0x20,	//00100000
	FDC_CMD_EXT_DENSITY		=	0x40,	//01000000
	FDC_CMD_EXT_MULTITRACK	=	0x80	//10000000
};


enum FLPYDSK_DOR_MASK {

	FLPYDSK_DOR_MASK_DRIVE0			=	0,	//00000000	= here for completeness sake
	FLPYDSK_DOR_MASK_DRIVE1			=	1,	//00000001
	FLPYDSK_DOR_MASK_DRIVE2			=	2,	//00000010
	FLPYDSK_DOR_MASK_DRIVE3			=	3,	//00000011
	FLPYDSK_DOR_MASK_RESET			=	4,	//00000100
	FLPYDSK_DOR_MASK_DMA			=	8,	//00001000
	FLPYDSK_DOR_MASK_DRIVE0_MOTOR	=	16,	//00010000
	FLPYDSK_DOR_MASK_DRIVE1_MOTOR	=	32,	//00100000
	FLPYDSK_DOR_MASK_DRIVE2_MOTOR	=	64,	//01000000
	FLPYDSK_DOR_MASK_DRIVE3_MOTOR	=	128	//10000000
};


enum FLPYDSK_MSR_MASK {

	FLPYDSK_MSR_MASK_DRIVE1_POS_MODE	=	1,	//00000001
	FLPYDSK_MSR_MASK_DRIVE2_POS_MODE	=	2,	//00000010
	FLPYDSK_MSR_MASK_DRIVE3_POS_MODE	=	4,	//00000100
	FLPYDSK_MSR_MASK_DRIVE4_POS_MODE	=	8,	//00001000
	FLPYDSK_MSR_MASK_BUSY				=	16,	//00010000
	FLPYDSK_MSR_MASK_DMA				=	32,	//00100000
	FLPYDSK_MSR_MASK_DATAIO				=	64, //01000000
	FLPYDSK_MSR_MASK_DATAREG			=	128	//10000000
};



enum FLPYDSK_ST0_MASK {

	FLPYDSK_ST0_MASK_DRIVE0		=	0,		//00000000	=	for completness sake
	FLPYDSK_ST0_MASK_DRIVE1		=	1,		//00000001
	FLPYDSK_ST0_MASK_DRIVE2		=	2,		//00000010
	FLPYDSK_ST0_MASK_DRIVE3		=	3,		//00000011
	FLPYDSK_ST0_MASK_HEADACTIVE	=	4,		//00000100
	FLPYDSK_ST0_MASK_NOTREADY	=	8,		//00001000
	FLPYDSK_ST0_MASK_UNITCHECK	=	16,		//00010000
	FLPYDSK_ST0_MASK_SEEKEND	=	32,		//00100000
	FLPYDSK_ST0_MASK_INTCODE	=	64		//11000000
};


enum FLPYDSK_ST0_INTCODE_TYP {

	FLPYDSK_ST0_TYP_NORMAL		=	0,
	FLPYDSK_ST0_TYP_ABNORMAL_ERR=	1,
	FLPYDSK_ST0_TYP_INVALID_ERR	=	2,
	FLPYDSK_ST0_TYP_NOTREADY	=	3
};

enum FLPYDSK_GAP3_LENGTH {

	FLPYDSK_GAP3_LENGTH_STD = 42,
	FLPYDSK_GAP3_LENGTH_5_14= 32,
	FLPYDSK_GAP3_LENGTH_3_5= 27
};


enum FLPYDSK_SECTOR_DTL {

	FLPYDSK_SECTOR_DTL_128	=	0,
	FLPYDSK_SECTOR_DTL_256	=	1,
	FLPYDSK_SECTOR_DTL_512	=	2,
	FLPYDSK_SECTOR_DTL_1024	=	4
};

//! floppy irq
//const int FLOPPY_IRQ = 6;

//! sectors per track
const int FLPY_SECTORS_PER_TRACK = 18;

//! dma tranfer buffer starts here and ends at 0x1000+64k
//! You can change this as needed. It must be below 16MB and in idenitity mapped memory!
void *dma_buffer;
//const int DMA_BUFFER = (int)dma_buffer;

//============================================================================
//    IMPLEMENTATION PRIVATE CLASS PROTOTYPES / EXTERNAL CLASS REFERENCES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE STRUCTURES / UTILITY CLASSES
//============================================================================
//============================================================================
//    IMPLEMENTATION REQUIRED EXTERNAL REFERENCES (AVOID)
//============================================================================

//! used to wait in miliseconds
//extern void sleep (int);

//============================================================================
//    IMPLEMENTATION PRIVATE DATA
//============================================================================

//! current working drive. Defaults to 0 which should be fine on most systems
static u8int	_CurrentDrive = 0;

//! set when IRQ fires
//static volatile u8int _FloppyDiskIRQ = 0;

//============================================================================
//    INTERFACE DATA
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTION PROTOTYPES
//============================================================================
//============================================================================
//    IMPLEMENTATION PRIVATE FUNCTIONS
//============================================================================


//! initialize DMA to use phys addr 84k-128k
void flpydsk_initialize_dma () {

	outportb (0x0a,0x06);	//mask dma channel 2
	outportb (0xd8,0xff);	//reset master flip-flop
	outportb ((uint)dma_buffer / 0x400, 0);     //address=0x1000 
	outportb (0x04, 0x10);
	outportb (0xd8, 0xff);  //reset master flip-flop
	outportb (0x05, 0xff);  //count to 0x23ff (number of bytes in a 3.5" floppy disk track)
	outportb (0x05, 0x23);
	outportb (0x80, 0);     //external page register = 0
	outportb (0x0a, 0x02);  //unmask dma channel 2
}

//! prepare the DMA for read transfer
void flpydsk_dma_read () {

	outportb (0x0a, 0x06); //mask dma channel 2
	outportb (0x0b, 0x56); //single transfer, address increment, autoinit, read, channel 2
	outportb (0x0a, 0x02); //unmask dma channel 2
}

//! prepare the DMA for write transfer
void flpydsk_dma_write () {

	outportb (0x0a, 0x06); //mask dma channel 2
	outportb (0x0b, 0x5a); //single transfer, address increment, autoinit, write, channel 2
	outportb (0x0a, 0x02); //unmask dma channel 2
}


//! return fdc status
u8int flpydsk_read_status () {

	//! just return main status register
	return inportb (FLPYDSK_MSR);
}

//! write to the fdc dor
void flpydsk_write_dor (u8int val ) {

	//! write the digital output register
	outportb (FLPYDSK_DOR, val);
}

//! send command byte to fdc
void flpydsk_send_command (u8int cmd) {

	//! wait until data register is ready. We send commands to the data register
	for (int i = 0; i < 500; i++ )
		if ( flpydsk_read_status () & FLPYDSK_MSR_MASK_DATAREG )
			return outportb (FLPYDSK_FIFO, cmd);
}

//! get data from fdc
u8int flpydsk_read_data () {

	//! same as above function but returns data register for reading
	for (int i = 0; i < 500; i++ )
		if ( flpydsk_read_status () & FLPYDSK_MSR_MASK_DATAREG )
			return inportb (FLPYDSK_FIFO);

	return 0;
}

//! write to the configuation control register
void flpydsk_write_ccr (u8int val) {

	//! write the configuation control
	outportb (FLPYDSK_CTRL, val);
}


//! wait for irq to fire
void flpydsk_wait_irq () {

	//! wait for irq to fire
	while ( FloppyIRQFlag == 0);
	FloppyIRQFlag = 0;
}


//!	floppy disk irq handler
//void i86_flpy_irq () {

//	_asm add esp, 12
//	_asm pushad
//	_asm cli

	//! irq fired
//	FloppyDiskIRQ = 1;

	//! tell hal we are done
//	interruptdone( FLOPPY_IRQ );

//	_asm sti
//	_asm popad
//	_asm iretd
//}

//! check interrupt status command
void flpydsk_check_int (uint* st0, uint* cyl) {

	flpydsk_send_command (FDC_CMD_CHECK_INT);

	*st0 = flpydsk_read_data ();
	*cyl = flpydsk_read_data ();
}

//! turns the current floppy drives motor on/off
void flpydsk_control_motor (uint b) {

	//! sanity check: invalid drive
	if (_CurrentDrive > 3)
		return;

	uint motor = 0;

	//! select the correct mask based on current drive
	switch (_CurrentDrive) {

		case 0:
			motor = FLPYDSK_DOR_MASK_DRIVE0_MOTOR;
			break;
		case 1:
			motor = FLPYDSK_DOR_MASK_DRIVE1_MOTOR;
			break;
		case 2:
			motor = FLPYDSK_DOR_MASK_DRIVE2_MOTOR;
			break;
		case 3:
			motor = FLPYDSK_DOR_MASK_DRIVE3_MOTOR;
			break;
	}

	//! turn on or off the motor of that drive
	if (b)
		flpydsk_write_dor (_CurrentDrive | motor | FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
	else
		flpydsk_write_dor (FLPYDSK_DOR_MASK_RESET);

	//! in all cases; wait a little bit for the motor to spin up/turn off

	for (int i = 0; i < 500; i++ )
		if ( flpydsk_read_status () & FLPYDSK_MSR_MASK_DATAREG ) return;
}

//! configure drive
void flpydsk_drive_data (uint stepr, uint loadt, uint unloadt, uint dma ) {

	uint data = 0;

	//! send command
	flpydsk_send_command (FDC_CMD_SPECIFY);
	data = ( (stepr & 0xf) << 4) | (unloadt & 0xf);
		flpydsk_send_command (data);
	data = (loadt) << 1 | (dma==0) ? 0 : 1;
		flpydsk_send_command (data);
}

//! calibrates the drive
int flpydsk_calibrate (uint drive) {

	uint st0, cyl;

	if (drive >= 4)
		return -2;

	//! turn on the motor
	flpydsk_control_motor (1);

	for (int i = 0; i < 10; i++) {

		//! send command
		flpydsk_send_command ( FDC_CMD_CALIBRATE );
		flpydsk_send_command ( drive );
		flpydsk_wait_irq ();
		flpydsk_check_int ( &st0, &cyl);

		//! did we fine cylinder 0? if so, we are done
		if (!cyl) {

			flpydsk_control_motor (0);
			return 0;
		}
	}

	flpydsk_control_motor (0);
	return -1;
}

//! disable controller
void flpydsk_disable_controller () {

	flpydsk_write_dor (0);
}

//! enable controller
void flpydsk_enable_controller () {

	flpydsk_write_dor ( FLPYDSK_DOR_MASK_RESET | FLPYDSK_DOR_MASK_DMA);
}

//! reset controller
void flpydsk_reset () {

	uint st0, cyl;

	//! reset the controller
	flpydsk_disable_controller ();
	flpydsk_enable_controller ();
	flpydsk_wait_irq ();

	//! send CHECK_INT/SENSE INTERRUPT command to all drives
	for (int i=0; i<4; i++)
		flpydsk_check_int (&st0,&cyl);

	//! transfer speed 500kb/s
	flpydsk_write_ccr (0);

	//! pass mechanical drive info. steprate=3ms, unload time=240ms, load time=16ms
	flpydsk_drive_data (3,16,240,1);

	//! calibrate the disk
	flpydsk_calibrate ( _CurrentDrive );
}

//! read a sector
void flpydsk_read_sector_imp (u8int head, u8int track, u8int sector) {

	uint st0, cyl;

	//! set the DMA for read transfer
	flpydsk_dma_read ();

	//! read in a sector
	flpydsk_send_command (
				FDC_CMD_READ_SECT | FDC_CMD_EXT_MULTITRACK | FDC_CMD_EXT_SKIP | FDC_CMD_EXT_DENSITY);
	flpydsk_send_command ( head << 2 | _CurrentDrive );
	flpydsk_send_command ( track);
	flpydsk_send_command ( head);
	flpydsk_send_command ( sector);
	flpydsk_send_command ( FLPYDSK_SECTOR_DTL_512 );
	flpydsk_send_command ( ( ( sector + 1 ) >= FLPY_SECTORS_PER_TRACK ) ? FLPY_SECTORS_PER_TRACK : sector + 1 );
	flpydsk_send_command ( FLPYDSK_GAP3_LENGTH_3_5 );
	flpydsk_send_command ( 0xff );

	//! wait for irq
	flpydsk_wait_irq ();

	//! read status info
	for (int j=0; j<7; j++)
		flpydsk_read_data ();

	//! let FDC know we handled interrupt
	flpydsk_check_int (&st0,&cyl);
}

//! seek to given track/cylinder
int flpydsk_seek ( uint cyl, uint head ) {

	uint st0, cyl0;

	if (_CurrentDrive >= 4)
		return -1;

	for (int i = 0; i < 10; i++ ) {

		//! send the command
		flpydsk_send_command (FDC_CMD_SEEK);
		flpydsk_send_command ( (head) << 2 | _CurrentDrive);
		flpydsk_send_command (cyl);

		//! wait for the results phase IRQ
		flpydsk_wait_irq ();
		flpydsk_check_int (&st0,&cyl0);

		//! found the cylinder?
		if ( cyl0 == cyl)
			return 0;
	}

	return -1;
}

//============================================================================
//    INTERFACE FUNCTIONS
//============================================================================

//! convert LBA to CHS
void flpydsk_lba_to_chs (int lba,int *head,int *track,int *sector) {

   *head = ( lba % ( FLPY_SECTORS_PER_TRACK * 2 ) ) / ( FLPY_SECTORS_PER_TRACK );
   *track = lba / ( FLPY_SECTORS_PER_TRACK * 2 );
   *sector = lba % FLPY_SECTORS_PER_TRACK + 1;
}

//! install floppy driver

void flpydsk_install (int irq) {

	//! install irq handler
//	setvect (irq, i86_flpy_irq);

	//! initialize the DMA for FDC
	flpydsk_initialize_dma ();

	//! reset the fdc
	flpydsk_reset ();

	//! set drive information
	flpydsk_drive_data (13, 1, 0xf, 1);
}

//! set current working drive
void flpydsk_set_working_drive (u8int drive) {

	if (drive < 4)
		_CurrentDrive = drive;
}

//! get current working drive
u8int flpydsk_get_working_drive () {

	return _CurrentDrive;
}

//! read a sector
u8int* flpydsk_read_sector (int sectorLBA) {

	if (_CurrentDrive >= 4)
		return 0;

	//! convert LBA sector to CHS
	int head=0, track=0, sector=1;
	flpydsk_lba_to_chs (sectorLBA, &head, &track, &sector);

	//! turn motor on and seek to track
	flpydsk_control_motor (1);
	if (flpydsk_seek (track, head) != 0)
		return 0;

	//! read sector and turn motor off
	flpydsk_read_sector_imp (head, track, sector);
	flpydsk_control_motor (0);

	//! warning: this is a bit hackish
//	return (u8int*) DMA_BUFFER;
	return (u8int*) dma_buffer;
}


void init_floppy() {
	register_interrupt_handler(IRQ6, &floppy_IRQ_handler);

	//! initialize the DMA for FDC
	flpydsk_initialize_dma ();

	//! reset the fdc
	flpydsk_reset ();

	//! set drive information
	flpydsk_drive_data (13, 1, 0xf, 1);

}


void floppy_test_read() {
    debug_i("Reading floppy ", 0);
    dma_buffer = (void*)kmalloc_a(65536, 0);
    memset(dma_buffer, 0xFF, 65536);
    FloppyIRQFlag = 0;

    init_floppy();

    uint sectornum = 0;
    char sectornumbuf [4];
    u8int* sector = 0;
 
    //! read sector from disk
    sector = flpydsk_read_sector ( sectornum );
    debug_i("Done " , (uint)sector);
    debug_i("DMA buffer: ", (uint)dma_buffer);
//    sector = 0x1000;

//    debug_i("Contents of sector ", sectornum);

    dump_mem(sector, 176, 13);
 
    //! display sector
    if (sector!=0) {
		return; 
        char txt[5] = "0x  ";
        int i = 0;
        for ( int c = 0; c < 4; c++ ) {
 
            for (int j = 0; j < 128; j++)
            	if (sector[i+j] == 0)
                debug(".");
//                txt[3] = sector[i+j];
                else debug_i("Data : ", sector[i+j]);
//                DebugPrintf ("0x%x ", sector[ i + j ]);
            i += 128;
 
        }
    }
    else
        debug("*** Error reading sector from disk");
}
