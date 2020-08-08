
//****************************************************************************
// serial.c
//
// (c) Hewell Technology Ltd. 2014
//
//****************************************************************************


#define  _DEFAULT_SOURCE // for cfmakeraw()

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

static FILE *log_fp = NULL;

#ifdef __WXMSW__
    #include <windows.h>
    #include <winerror.h>

    // To get windows device functions
    #include <Setupapi.h>
    #include <devguid.h>

    static HANDLE handle = INVALID_HANDLE_VALUE;
#else // __WXMSW__
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <linux/serial.h>
    #include <sys/ioctl.h>

    static int fd = -1;
#endif

int is_data_available(int fd);

static char error_str[128] = "";

//***************************************************************************
static void debug_log(const uint8_t *buf, uint16_t len, bool out)
{
    if (log_fp != NULL)
    {
        fprintf(log_fp, "%s ", out ? "\n-->" : "<--");

        for(int i = 0; i < len; i++)
         fprintf(log_fp, "%02X ", buf[i]);

        fprintf(log_fp, "\n");
    }

    fflush(log_fp);
}

//***************************************************************************
void debug_comment(const char *comment, ...)
{
    if (log_fp != NULL)
    {
        va_list ap;

        va_start(ap, comment);
        vfprintf(log_fp, comment, ap);
        va_end(ap);
    }
}

//***************************************************************************
const char *serial_get_error(void)
{
    return error_str;
}

//***************************************************************************
bool serial_open_port(const char *port)
{
    bool ret = false;

#ifdef __WXMSW__
    if (handle == INVALID_HANDLE_VALUE)
    {
		if (*port > 0)
        {
            char device[64];
            snprintf(device, sizeof(device), "\\\\.\\COM%d", *port);

            handle = CreateFile(device,
                                GENERIC_READ | GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);

            if (handle != INVALID_HANDLE_VALUE) {
                ret = true;
            }
            else
            {
                strncpy(error_str, "Error opening COM port", sizeof(error_str));
            }
        }
        else
        {
         strncpy(error_str, "Invalid COM port", sizeof(error_str));
        }
    }
    else
    {
        strncpy(error_str, "Port already open", sizeof(error_str));
    }

    log_fp = fopen("debug.log", "w");
#else  // !__WXMSW__
    if (fd < 0)
    {
		fd = open(port, O_RDWR | O_NONBLOCK); //new open file "DESCRIPTION"!
        if (fd >= 0) {
			// Port Available
            ret = true;
        } else {
			// Port NOT Available
        	ret = false;
        }
    }
    else
    {
        strncpy(error_str, "Port already open", sizeof(error_str));
    }
#endif // __WXMSW__

    return ret;
}

#ifndef __WXMSW__
//***************************************************************************
int rate_to_constant(int baudrate) {
#define B(x) case x: return B##x
	switch(baudrate) {
		B(50);     B(75);     B(110);    B(134);    B(150);
		B(200);    B(300);    B(600);    B(1200);   B(1800);
		B(2400);   B(4800);   B(9600);   B(19200);  B(38400);
		B(57600);  B(115200); B(230400); B(460800); B(500000);
		B(576000); B(921600); B(1000000);B(1152000);B(1500000);
	default: return 0;
	}
#undef B
}
#endif // __WXMSW__


//***************************************************************************
bool serial_set_baud_rate(int rate)
{
    bool ret = false;
#ifdef __WXMSW__
    DCB dcb_config;

    if (handle != INVALID_HANDLE_VALUE)
    {
        if (GetCommState(handle, &dcb_config))
        {
            dcb_config.BaudRate = rate;
            dcb_config.ByteSize = 8;
            dcb_config.Parity = NOPARITY;
            dcb_config.StopBits = ONESTOPBIT;
            dcb_config.fBinary = TRUE;
            dcb_config.fParity = FALSE;
            dcb_config.fOutxCtsFlow = FALSE;
            dcb_config.fOutxDsrFlow = FALSE;
            dcb_config.fDtrControl = DTR_CONTROL_DISABLE;
            dcb_config.fDsrSensitivity = FALSE;
            dcb_config.fTXContinueOnXoff = TRUE;
            dcb_config.fOutX = FALSE;
            dcb_config.fInX = FALSE;
            dcb_config.fRtsControl = RTS_CONTROL_DISABLE;

            if (SetCommState(handle, &dcb_config))
            {
                COMMTIMEOUTS cto;

                if (GetCommTimeouts(handle, &cto))
                {
                    // Set non-blocking reads
                    cto.ReadIntervalTimeout = MAXDWORD;
                    cto.ReadTotalTimeoutConstant = 0;
                    cto.ReadTotalTimeoutMultiplier = 0;

                    SetCommTimeouts(handle, &cto);

                    ret = true;
                }
            }
            else
            {
            	strncpy(error_str, "Error configuring COM port", sizeof(error_str));
            }
        }
        else
        {
            strncpy(error_str, "Error reading COM port settings", sizeof(error_str));
        }
    }
    else
    {
        strncpy(error_str, "Port not open", sizeof(error_str));
    }
#else // !__WXMSW__

    struct termios attr;
    struct serial_struct serinfo;

    // (Not too sure what the below does?!)
    int speed = rate_to_constant(rate);

    if (speed == 0)
    {
        //Custom divisor
        serinfo.reserved_char[0] = 0;
        if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0)
        {
            return 0;
        }

        serinfo.flags &= ~ASYNC_SPD_MASK;
        serinfo.flags |= ASYNC_SPD_CUST;
        serinfo.custom_divisor = (serinfo.baud_base + (rate / 2)) / rate;

        if (serinfo.custom_divisor < 1)
        {
            serinfo.custom_divisor = 1;
        }

        if (ioctl(fd, TIOCSSERIAL, &serinfo) < 0)
        {
            strncpy(error_str, "Error with TIOCSSERIAL (Setting serial line information)", sizeof(error_str));
            return 0;
        }

        if (ioctl(fd, TIOCGSERIAL, &serinfo) < 0)
        {
            strncpy(error_str, "Error with TIOCGSERIAL (Getting serial line information)", sizeof(error_str));
            return 0;
        }

        if (serinfo.custom_divisor * rate != serinfo.baud_base)
        {
            // warnx("Actual baudrate is %d / %d = %f",
            //     serinfo.baud_base, serinfo.custom_divisor,
            //     (float)serinfo.baud_base / serinfo.custom_divisor);
        }
    }
    else
    {
        strncpy(error_str, "Error reading COM port settings", sizeof(error_str));
    }

    //Set baud rate
    fcntl(fd, F_SETFL, O_NONBLOCK); //Sets file "DESCRIPTOR" to open
    tcgetattr(fd, &attr);
    /*Currently this code has only been tested when using the non-standard baud
    rate of 625000. Should you use a baud rate other than this, there will
    likely be errors. the functions below would likely need changing.*/
    cfsetispeed(&attr, speed ?: B38400);
    cfsetospeed(&attr, speed ?: B38400);
    cfmakeraw(&attr);   /* set serial port to raw mode. Need to move this to own
                        function call*/
    //attr.c_cflag |= (CLOCAL | CREAD);
    //attr.c_cflag &= ~CRTSCTS;
    //attr.c_lflag &= ~ICANON; // Set non-canonical mode
    attr.c_cc[VMIN] = 1;
    attr.c_cc[VTIME] = 10; // Set timeout of 0.1 seconds

    // Input flags - Turn off input processing
    // convert break to null byte, no CR to NL translation,
    // no NL to CR translation, don't mark parity errors or breaks
    // no input parity check, don't strip high bit off,
    // no XON/XOFF software flow control
    //
    attr.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |
                        INLCR | PARMRK | INPCK |
                        ISTRIP | IXON | IXOFF | IXANY);
    //
    // Output flags - Turn off output processing
    // no CR to NL translation, no NL to CR-NL translation,
    // no NL to CR translation, no column 0 CR suppression,
    // no Ctrl-D suppression, no fill characters, no case mapping,
    // no local output processing
    //
    // config.c_oflag &= ~(OCRNL | ONLCR | ONLRET |
    //                     ONOCR | ONOEOT| OFILL | OLCUC | OPOST);
    attr.c_oflag = 0;
    //
    // No line processing:
    // echo off, echo newline off, canonical mode off,
    // extended input processing off, signal chars off
    //
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
    //
    // Turn off character processing
    // clear current char size mask, no parity checking,
    // no output processing, force 8 bit input

    attr.c_cflag |= (CLOCAL | CREAD | CS8);
    attr.c_cflag &= ~(CSIZE | CRTSCTS | PARENB);
    if (tcsetattr(fd, TCSANOW, &attr) != 0)
    {
        strncpy(error_str, "Error reading COM port settings", sizeof(error_str));
    }
    else
    {
        ret = true;
    }
#endif // __WXMSW__

    return ret;
}

//***************************************************************************
bool serial_close_port(void)
{
    #ifdef __WXMSW__
        if (handle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
    #else // __WXMSW__
        if (fd >= 0)
        {
            close(fd);
            fd = -1;
        }
    #endif // __WXMSW__

    if (log_fp != NULL)
    {
        fclose(log_fp);
        log_fp = NULL;
    }
    return true;
}


//***************************************************************************
ssize_t serial_write(const void *buf, size_t count)
{
    ssize_t ret = -1;
    #ifdef __WXMSW__
        DWORD written;
        if (WriteFile(handle, buf, count, &written, NULL))
        {
            ret = written;
        }
    #else
        ret = write(fd, buf, count);
    #endif

	debug_log(buf, ret, true);
    return ret;
}

//***************************************************************************
ssize_t serial_read(void *buf, size_t count)
{
    ssize_t ret = -1;
    ssize_t bytes_recieved;
    void *buf_ptr = buf;
    ssize_t original_count = count;

    while (count > 0)
    {
        int status_data_available = is_data_available(fd);
        // Data may be available...
        if (status_data_available > 0)
        {
            #ifdef __WXMSW__
                DWORD read;
                if (ReadFile(handle, buf, count, &read, NULL)) {
                    bytes_recieved = read;
                }
            #else
                bytes_recieved = read(fd, buf_ptr, count);
            #endif
            buf_ptr += bytes_recieved;
            count -= bytes_recieved;
        }
        else if (status_data_available == 0)
        {
            // Timed out - nothing to read!
            // Hence "original_count" will equal "count"
            break;
        }
        else if (status_data_available < 0)
        {
            // Error in SELECT function
            // Hope this doesn't happen!
            return -1;
        }
    }
    ret = original_count - count; //For a data packet this value should be 247
	debug_log(buf, ret, false);
    return ret;
}

//***************************************************************************
int is_data_available(int fd)
{
    #ifdef __WXMSW__
    	return 1;
    #else // !__WXMSW__
    	fd_set rd, err;

    	struct timeval wait = {1, 0};  //Wait 1 second
    	FD_ZERO(&rd);
    	FD_SET(fd, &rd);

    	int ret = select(fd + 1, &rd, NULL, &err, &wait);

    	// Timeout expired...
    	if (ret == 0)
        {
    		return ret;
    	}
    	if (ret > 0)
        {
    		//An event is pending
    		if (FD_ISSET(fd, &rd))
            {
    			return ret;  // A "read" is now available
    		}
    	}
        else if (ret < 0)
        {
    		// An EINTR error occurred, just print it to stdout
    		if (errno != EINTR)
            {
    			perror("select");
    			printf("Error on select(): %s\n", strerror(errno));
    			errno = 0;
    			ret = -1;
    		}
            else
            {
    			//Ignore EINTR
    			//errno = 0;
    			//perror("Xselect");
    			//printf("XError on select(): %s\n", strerror(errno));
    			errno = 0;
    			ret = 0; //Ignore EINTR, treat as a timeout
    		}
    	}

    	return ret;
    #endif // __WXMSW__
}


