#ifndef XR21B1411_XRDL
#define XR21B1411_XRDL

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* XR21B1411                                                                                                                          */
/* The Mongo address map contains RAM, ROM and registers. The RAM is used for various purposes including TX and RX FIFOs for
   bulk data, and buffers for USB control messages. It is also used by the PRM SPIT for working memory. The ROM contains USB
   descriptors and baud rate tables. The high order bits of the address are used to select between these targets. All locations are
   accessible from the PRM, and using a vendor protocol message they are accessible from the host as well. */
/*--------------------------------------------------------------------------------------------------------------------------------*/
#define XR21B1411_RAM_START                  0x000       
#define XR21B1411_RAM_START_SIZE             0           
#define XR21B1411_RX_FIFO                    0x000       /* The RX FIFO. Written by UART RX path and read by USB Bulk IN transfers.
                                                        */
#define XR21B1411_RX_FIFO_SIZE               384         
#define XR21B1411_TX_FIFO                    0x180       /* The TX FIFO. Written by USB Bulk OUT transfers, and read by the UART TX
                                                        path. */
#define XR21B1411_TX_FIFO_SIZE               128         
#define XR21B1411_PRM_RAM                    0x200       /* The PRM SPIT uses memory for its working store. The detailed allocation
                                                        of its memory map is in fw/include/prm.xrdl. Make sure we know what
                                                        alignment this block has so that the PRM code knows what assumptions it can
                                                        make. It is currently 512-byte aligned, which is very nice. */
#define XR21B1411_PRM_RAM_SIZE               128         
#define XR21B1411_CONTROL_IN                 0x280       /* Buffer for USB Control IN transfers. Written by the PRM SPIT and read by
                                                        USB IN path. Access to this buffer is controlled by the CONTROL_IN_VALID
                                                        shag and the CONTROL_IN_CMD and CONTROL_IN_LEN registers. */
#define XR21B1411_CONTROL_IN_SIZE            64          
#define XR21B1411_CONTROL_OUT                0x2c0       /* Buffer for USB Control OUT transfers. Written by the USB OUT path and
                                                        read by the PRM SPIT. Access to this buffer is controlled by the
                                                        CONTROL_OUT_VALID and CONTROL_OUT_BUSY shags, and the CONTROL_OUT_CMD and
                                                        CONTROL_OUT_LEN registers. */
#define XR21B1411_CONTROL_OUT_SIZE           64          
#define XR21B1411_CONTROL_SETUP              0x300       /* Buffer used for USB SETUP packets. Written by the USB OUT path and read
                                                        by the PRM SPIT. Access to this buffer is controlled by the SETUP_CMD
                                                        register. It contains the eight bytes of SETUP data followed by one byte of
                                                        CMA discriminator. */
#define XR21B1411_CONTROL_SETUP_SIZE         9           
#define XR21B1411_RAM_FREE                   0x309       /* The first unused RAM location. */
#define XR21B1411_RAM_FREE_SIZE              0           
#define XR21B1411_RAM_END                    0x3ff       
#define XR21B1411_RAM_END_SIZE               0           
#define XR21B1411_ROM                        0x800       /* The configuration ROM contains USB descriptors (ROM_*). */
#define XR21B1411_ROM_SIZE                   1024        
#define XR21B1411_REG_START                  0xc00       
#define XR21B1411_REG_START_SIZE             0           
#define XR21B1411_REG_UART                   0xc00       /* UART status and control registers (UART_*). */
#define XR21B1411_REG_UART_SIZE              64          
#define XR21B1411_REG_CBM                    0xc40       /* Control Buffer Manager (CBM) status and control (CBM_*) registers. */
#define XR21B1411_REG_CBM_SIZE               64          
#define XR21B1411_REG_BFMTX                  0xc80       /* Bulk TX FIFO Manager (BFMTX) status and control (BFMTX_*) registers. */
#define XR21B1411_REG_BFMTX_SIZE             64          
#define XR21B1411_REG_BFMRX                  0xcc0       /* Bulk RX FIFO Manager (BFMRX) status and control (BFMRX_*) registers. */
#define XR21B1411_REG_BFMRX_SIZE             64          
#define XR21B1411_REG_EPM                    0xd00       /* Endpoint Manager status and control (EPM_*) registers. */
#define XR21B1411_REG_EPM_SIZE               64          
#define XR21B1411_REG_OTP                    0xd40       /* One-Time Programmable manager status and control (OTP_*) registers. */
#define XR21B1411_REG_OTP_SIZE               64          
#define XR21B1411_REG_END                    0xfff       
#define XR21B1411_REG_END_SIZE               0           

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* SHAG                                                                                                                           */
/* Shared flags are used as a synchronization mechanism between the SPIT PRM and other hardware engines. They are single-bit
   values that can be set and cleared by either PRM or the hardware. Each shag controls a specific function and the set and clear
   operations must follow the specific handshake protocol defined for that function. */
/*--------------------------------------------------------------------------------------------------------------------------------*/
#define SHAG_CONTROL_OUT_VALID           0x0         /* This shag is automatically set by CBM whenever CBM_CONTROL_OUT_CMD_VALID
                                                        is set and CBM_CONTROL_OUT_CMD_SEQ has the same sequence number as
                                                        CBM_SETUP_CMD_SEQ. It is cleared automatically by CBM otherwise. This shag
                                                        is polled by PRM to discover that a new Control data stage is ready for
                                                        processing. */
#define SHAG_CONTROL_OUT_BUSY            0x1         /* This shag prevents CBM from overwriting Control OUT data while the PRM
                                                        is processing it. It is set by PRM when it starts to process the contents
                                                        of the CONTROL_OUT buffer. PRM must clear this shag when it has finished
                                                        processing the data (it must also indirecty clear SHAG_CONTROL_OUT_VALID by
                                                        writing a zero to CBM_CONTROL_OUT_CMD_VALID). */
#define SHAG_CONTROL_IN_VALID            0x2         /* This shag is automatically set by CBM whenever CBM_CONTROL_IN_CMD_VALID
                                                        is set and CBM_CONTROL_IN_CMD_SEQ has the same sequence number as
                                                        CBM_SETUP_CMD_SEQ. It is cleared automatically by CBM otherwise. */
#define SHAG_TOGGLE_BULK_IN_RESET        0x3         /* The PRM sets this shag to request the EPM to reset the BULK_IN DATA
                                                        toggle value. This happens during SET_CONFIGURATION and CLEAR_HALT
                                                        operations. After setting it, the PRM polls the shag waiting for the EPM to
                                                        clear it, indicating that the toggle reset has been done. */
#define SHAG_TOGGLE_BULK_OUT_RESET       0x4         /* The PRM sets this shag to request the EPM to reset the BULK_OUT DATA
                                                        toggle value. This usually happens during SET_CONFIGURATION and CLEAR_HALT
                                                        operations. After setting it, the PRM polls the shag waiting for the EPM to
                                                        clear it, indicating that the toggle reset has been done. */
#define SHAG_TOGGLE_INT_IN_RESET         0x5         /* The PRM sets this shag to request the EPM to reset the INT_IN DATA
                                                        toggle value. This usually happens during SET_CONFIGURATION and CLEAR_HALT
                                                        operations. After setting it, the PRM polls the shag waiting for the EPM to
                                                        clear it, indicating that the toggle reset has been done. */
#define SHAG_SIZE                        0x6         /* This is not a actual physical register. It's used as a variable to
                                                        determine the actual size of the shag register. */

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* ROM                                                                                                                            */
/* The ROM contains USB descriptors. All locations are accessible from the PRM, and using a vendor protocol message they are
   accessible from the host as well. At start up all values from LINE_CODING through the end of SERIAL_NUMBER_STRING inclusive are
   copied into RAM. This means that the RAM map must match the dsciprion here for that region. */
/*--------------------------------------------------------------------------------------------------------------------------------*/
#define ROM_START                        0x000       
#define ROM_START_SIZE                   0           
#define ROM_LINE_CODING                  0x000       /* The default baud rate, character size and parity used to configure the
                                                        UART at initialization. */
#define ROM_LINE_CODING_SIZE             7           
#define ROM_USB_VENDOR_ID_LSB            0x007       /* Bits [7:0] of the USB vendor ID reported with the descriptors. */
#define ROM_USB_VENDOR_ID_LSB_SIZE       1           
#define ROM_USB_VENDOR_ID_MSB            0x008       /* Bits [15:8] of the USB vendor ID reported with the descriptors. */
#define ROM_USB_VENDOR_ID_MSB_SIZE       1           
#define ROM_USB_PRODUCT_ID_LSB           0x009       /* Bits [7:0] of the USB product ID reported with the descriptors. */
#define ROM_USB_PRODUCT_ID_LSB_SIZE      1           
#define ROM_USB_PRODUCT_ID_MSB           0x00a       /* Bits [15:8] of the USB product ID reported with the descriptors. */
#define ROM_USB_PRODUCT_ID_MSB_SIZE      1           
#define ROM_USB_ATTRIBUTES               0x00b       /* The default remote wakeup and self-powered USB flags. */
#define ROM_USB_ATTRIBUTES_SIZE          1           
#define ROM_USB_MAXPOWER                 0x00c       /* The bMaxPower field of the device descriptor. It is expressed in units
                                                        of 2 mA */
#define ROM_USB_MAXPOWER_SIZE            1           
#define ROM_MANUFACTURER_STRING          0x00d       /* The default manufacturer string. Null-terminated ASCII. */
#define ROM_MANUFACTURER_STRING_SIZE     16          
#define ROM_PRODUCT_STRING               0x01d       /* The default product string. Null-terminated ASCII. */
#define ROM_PRODUCT_STRING_SIZE          16          
#define ROM_SERIAL_NUMBER_STRING         0x02d       /* The default serial number string. Null-terminated ASCII. */
#define ROM_SERIAL_NUMBER_STRING_SIZE    16          
#define ROM_CORE_CLOCK_RATE              0x03d       /* Initialized at the beginning of time to have the core clock rate in
                                                        hertz. This value is calculated from the XR21B1411_CONFIG3_CORE_CLOCK_DIV value
                                                        stored in the OTP. The three bytes are stored in little-endian format. */
#define ROM_CORE_CLOCK_RATE_SIZE         3           
#define ROM_CORE_CLOCKS_PER_US           0x040       /* Initialized at the beginning of time to have the number of clocks the
                                                        PRM executes in one microsecond (the value is actually adjusted to take
                                                        account of some of the overheads in the WAIT_N_US code). This value is
                                                        calculated from the XR21B1411_CONFIG3_CORE_CLOCK_DIV value stored in the OTP.
                                                        */
#define ROM_CORE_CLOCKS_PER_US_SIZE      1           
#define ROM_UART_MASK_TABLE              0x080       /* The table of masks for the baud rate calculation code. It must be
                                                        64-byte aligned. There are four 16-byte blocks of masks in this order: RX
                                                        odd, TX odd, RX even, TX even. */
#define ROM_UART_MASK_TABLE_SIZE         64          
#define ROM_CFG_DESC                     0x200       /* The 75 bytes of USB configuration descriptor information is stored in
                                                        this location. It contain not only the configuration descriptor but also
                                                        interface and endpoint descriptor information. The final value will be
                                                        determined by the OTP value. */
#define ROM_CFG_DESC_SIZE                75          
#define ROM_DEV_DESC                     0x280       /* The 18 bytes of USB device descriptor information is stored in this
                                                        location. The final description information will also be determined by the
                                                        OTP value. */
#define ROM_DEV_DESC_SIZE                18          
#define ROM_BIST_DEV_DESC                0x2a0       /* When the device is in BIST mode, the 18 bytes of USB device descriptor
                                                        information is stored in this location. */
#define ROM_BIST_DEV_DESC_SIZE           18          
#define ROM_BIST_CFG_DESC                0x2c0       /* When the device is in BIST mode, the 18 bytes of USB configuration
                                                        descriptor information is stored in this location. */
#define ROM_BIST_CFG_DESC_SIZE           18          
#define ROM_FREE_START                   0x2d2       /* The first unused ROM location. */
#define ROM_FREE_START_SIZE              0           
#define ROM_END                          0x3ff       
#define ROM_END_SIZE                     0           

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* UART                                                                                                                           */
/* A tentative set of UART registers, taken from the Vizzini UART. They are not necessarily at the same address that they are in
   Vizzini, however. */
/*--------------------------------------------------------------------------------------------------------------------------------*/

#define UART_ENABLE                      0x00        
#define UART_ENABLE_TX                   0x1         /* Enable the UART transmitter. */
#define UART_ENABLE_TX__S                0           
#define UART_ENABLE_TX__M                0x1         
#define UART_ENABLE_RX                   0x2         /* Enable the UART receiver. */
#define UART_ENABLE_RX__S                1           
#define UART_ENABLE_RX__M                0x1         

#define UART_CLOCK_DIVISOR_0             0x01        /* Bits[11:0] of the baud rate generator divisor. Default is for 8 Mbps
                                                        baud rate for 48 MHz core clock. */
#define UART_CLOCK_DIVISOR_0_RESETVAL    0x006       

#define UART_CLOCK_DIVISOR_1             0x02        /* Bits[19:12] of the baud rate generator divisor, and diagnostic mode. */
#define UART_CLOCK_DIVISOR_1_MSB         0xff        /* Bits[19:12] of the baud rate generator divisor. */
#define UART_CLOCK_DIVISOR_1_MSB__S      0           
#define UART_CLOCK_DIVISOR_1_MSB__M      0xff        
#define UART_CLOCK_DIVISOR_1_DIAGMODE    0x100       /* Enable diagnostic mode. NOTE: if DIAGMODE is used, the lower 10 bits of
                                                        the clock divisor have to be zero for proper operation. */
#define UART_CLOCK_DIVISOR_1_DIAGMODE__S 8           
#define UART_CLOCK_DIVISOR_1_DIAGMODE__M 0x1         

#define UART_TX_CLOCK_MASK               0x03        /* The baud rate generator TX mask (12 bits). */
#define UART_RX_CLOCK_MASK               0x04        /* The baud rate generator RX mask (12 bits). */

#define UART_FORMAT                      0x05        /* Controls the character format. */
#define UART_FORMAT_SIZE                 0xf         /* The number of data bits per character. Legal values are 7, 8, 9 bits. */
#define UART_FORMAT_SIZE__S              0           
#define UART_FORMAT_SIZE__M              0xf         
#define UART_FORMAT_SIZE_D7              0x7         
#define UART_FORMAT_SIZE_D8              0x8         
#define UART_FORMAT_SIZE_D9              0x9         
#define UART_FORMAT_PARITY               0x70        /* The parity applied to the character. */
#define UART_FORMAT_PARITY__S            4           
#define UART_FORMAT_PARITY__M            0x7         
#define UART_FORMAT_PARITY_NONE          0x0         
#define UART_FORMAT_PARITY_ODD           0x1         
#define UART_FORMAT_PARITY_EVEN          0x2         
#define UART_FORMAT_PARITY_MARK          0x3         
#define UART_FORMAT_PARITY_SPACE         0x4         
#define UART_FORMAT_STOP                 0x80        /* The number of stop bits per character. Legal values are 1, 2 bits. */
#define UART_FORMAT_STOP__S              7           
#define UART_FORMAT_STOP__M              0x1         
#define UART_FORMAT_STOP_ONE             0x0         
#define UART_FORMAT_STOP_TWO             0x1         
#define UART_FORMAT_RESETVAL             0x008       

#define UART_FLOW                        0x06        /* Controls the flow control mode and whether half-duplex operation is
                                                        enabled. */
#define UART_FLOW_MODE                   0x7         /* Flow control mode. */
#define UART_FLOW_MODE__S                0           
#define UART_FLOW_MODE__M                0x7         
#define UART_FLOW_MODE_NONE              0x0         /* None */
#define UART_FLOW_MODE_HW                0x1         /* Hardware flow control */
#define UART_FLOW_MODE_SW                0x2         /* Software flow control (XON/XOFF) */
#define UART_FLOW_MODE_ADDR_MATCH        0x3         /* Address match mode */
#define UART_FLOW_MODE_ADDR_MATCH_TX     0x4         /* Address match with TX flow control */
#define UART_FLOW_HALF_DUPLEX            0x8         /* Enable half-duplex opertation. */
#define UART_FLOW_HALF_DUPLEX__S         3           
#define UART_FLOW_HALF_DUPLEX__M         0x1         

#define UART_XON_CHAR                    0x07        /* The character to be used to resume transmission when software flow
                                                        control is enabled. The unicast address used in address match mode. */
#define UART_XON_CHAR_VAL                0xff        /* Any Xon character is at most 8 bits. */
#define UART_XON_CHAR_VAL__S             0           
#define UART_XON_CHAR_VAL__M             0xff        
#define UART_XON_CHAR_RESETVAL           0x011       

#define UART_XOFF_CHAR                   0x08        /* The character to be used to stop transmission when software flow control
                                                        is enabled. The multicast address used in address match mode. */
#define UART_XOFF_CHAR_VAL               0xff        /* Any Xoff character is at most 8 bits. */
#define UART_XOFF_CHAR_VAL__S            0           
#define UART_XOFF_CHAR_VAL__M            0xff        
#define UART_XOFF_CHAR_RESETVAL          0x013       

#define UART_ERR_STATUS                  0x09        /* The UART error status. */
#define UART_ERR_STATUS_BREAK            0x8         /* A break condition has been detected. A break condition is declared when
                                                        all the data bits and the first stop bit are all zeros. This value is
                                                        sticky (i.e. it remains set even when the break condition goes away. A
                                                        break condition goes away when the UART line returns to idle, i.e. high),
                                                        and is cleared when read. */
#define UART_ERR_STATUS_BREAK__S         3           
#define UART_ERR_STATUS_BREAK__M         0x1         
#define UART_ERR_STATUS_FRAME            0x10        /* A framing error has occurred. A framing error is declared when the first
                                                        stop bit of a character is incorrect (i.e. low). This value is sticky and
                                                        is cleared when read. */
#define UART_ERR_STATUS_FRAME__S         4           
#define UART_ERR_STATUS_FRAME__M         0x1         
#define UART_ERR_STATUS_PARITY           0x20        /* A parity error has occurred. This value is sticky (it remains set even
                                                        if a subsequent character is received without an error), and is cleared
                                                        when read. */
#define UART_ERR_STATUS_PARITY__S        5           
#define UART_ERR_STATUS_PARITY__M        0x1         
#define UART_ERR_STATUS_OVERRUN          0x40        /* Either RX FIFO overrun or UART receiver overrun has occurred. This value
                                                        is sticky and is cleared when read. */
#define UART_ERR_STATUS_OVERRUN__S       6           
#define UART_ERR_STATUS_OVERRUN__M       0x1         
#define UART_ERR_STATUS_BRK_STAT         0x80        /* Set when a break condition is currently being detected. NOTE: This bit
                                                        does not get cleared when this register is read. */
#define UART_ERR_STATUS_BRK_STAT__S      7           
#define UART_ERR_STATUS_BRK_STAT__M      0x1         

#define UART_TX_BREAK                    0x0a        /* This register controls whether the UART Tx shall send out a break
                                                        signal, and how long the break signal shall last. When the user writes a
                                                        number, N, to this register: if N == 0xfff, UART Tx sends a continuous
                                                        break signal; if 0x000 < N < 0xfff, UART Tx sends a break signal that lasts
                                                        N ms, and the register servers as a counter counting down to 0, decrement
                                                        by 1 every ms; if N == 0x0000, UART Tx stops sending the break signal. When
                                                        the user writes to this register, the previous effect is terminated right
                                                        away, and the new command takes effect. NOTE: after this register is
                                                        programmed from 0x000 to a non-zero value, the UART Tx may take up to but
                                                        no more than 1ms before starting to send out the break on the line. In
                                                        addition, after the break counter counts down to zero, the UART Tx may take
                                                        up to but no more than 2 UART characters, based on the current UART
                                                        configuration, before stopping the break. Thus, the actual break length
                                                        will be slightly longer, by up to but no more than (1ms + 2x
                                                        UART-character-length), than the programmed value. */

#define UART_XCVR_EN_DELAY               0x0b        /* Hold Transceiver Enable signal active for an additional number of baud
                                                        times. (0-15). When the UART determines that there are no more characters
                                                        to be transmitted it delays some number of baud bit times before
                                                        de-asserting the transceiver enable signal. */
#define UART_XCVR_EN_DELAY_VAL           0xf         /* The number of baud bit times to wait before de-asserting the transceiver
                                                        enable. */
#define UART_XCVR_EN_DELAY_VAL__S        0           
#define UART_XCVR_EN_DELAY_VAL__M        0xf         

#define UART_GPIO_MODE                   0x0c        /* Control special GPIO pin modes. */
#define UART_GPIO_MODE_SEL               0x7         /* GPIO pin mode selection. */
#define UART_GPIO_MODE_SEL__S            0           
#define UART_GPIO_MODE_SEL__M            0x7         
#define UART_GPIO_MODE_SEL_GPIO          0x0         /* All GPIO pins available as general purpose I/O. */
#define UART_GPIO_MODE_SEL_RTS_CTS       0x1         /* GPIO 5 used as RTS output, GPIO 4 used as CTS input, others GPIO. */
#define UART_GPIO_MODE_SEL_DTR_DSR       0x2         /* GPIO 3 used as DTR output, GPIO 2 used as DSR input, others GPIO. */
#define UART_GPIO_MODE_SEL_XCVR_EN_ACT   0x3         /* GPIO 5 used as transceiver enable. Asserted whenever the TX UART is
                                                        transmitting a character. */
#define UART_GPIO_MODE_SEL_XCVR_EN_FLOW  0x4         /* GPIO 5 used as transceiver enable. Used with Address match with TX flow
                                                        control mode. (See UART_FLOW_MODE) The Tranceiver Enable output is asserted
                                                        for the entire time a unicast address match condition exists. */
#define UART_GPIO_MODE_XCVR_EN_POL       0x8         /* Transceiver enable polarity. 0=Active low enable. 1=Active high enable.
                                                        */
#define UART_GPIO_MODE_XCVR_EN_POL__S    3           
#define UART_GPIO_MODE_XCVR_EN_POL__M    0x1         

#define UART_GPIO_DIRECTION              0x0d        /* Controls the direction of the GPIO pins. 1=output, 0=input. The
                                                        programming of UART_GPIO_MODE_SEL takes precedence over this register. */
#define UART_GPIO_DIRECTION_RI           0x1         /* Direction of GPIO 0 */
#define UART_GPIO_DIRECTION_RI__S        0           
#define UART_GPIO_DIRECTION_RI__M        0x1         
#define UART_GPIO_DIRECTION_CD           0x2         /* Direction of GPIO 1 */
#define UART_GPIO_DIRECTION_CD__S        1           
#define UART_GPIO_DIRECTION_CD__M        0x1         
#define UART_GPIO_DIRECTION_DSR          0x4         /* Direction of GPIO 2 */
#define UART_GPIO_DIRECTION_DSR__S       2           
#define UART_GPIO_DIRECTION_DSR__M       0x1         
#define UART_GPIO_DIRECTION_DTR          0x8         /* Direction of GPIO 3 */
#define UART_GPIO_DIRECTION_DTR__S       3           
#define UART_GPIO_DIRECTION_DTR__M       0x1         
#define UART_GPIO_DIRECTION_CTS          0x10        /* Direction of GPIO 4 */
#define UART_GPIO_DIRECTION_CTS__S       4           
#define UART_GPIO_DIRECTION_CTS__M       0x1         
#define UART_GPIO_DIRECTION_RTS          0x20        /* Direction of GPIO 5 */
#define UART_GPIO_DIRECTION_RTS__S       5           
#define UART_GPIO_DIRECTION_RTS__M       0x1         

#define UART_GPIO_SET                    0x0e        /* Set GPIO output. Writing a 1 to a bit in this register drives the GPIO
                                                        output high. Writing a 0 to a bit has no effect. */
#define UART_GPIO_SET_RI                 0x1         /* Set GPIO 0 */
#define UART_GPIO_SET_RI__S              0           
#define UART_GPIO_SET_RI__M              0x1         
#define UART_GPIO_SET_CD                 0x2         /* Set GPIO 1 */
#define UART_GPIO_SET_CD__S              1           
#define UART_GPIO_SET_CD__M              0x1         
#define UART_GPIO_SET_DSR                0x4         /* Set GPIO 2 */
#define UART_GPIO_SET_DSR__S             2           
#define UART_GPIO_SET_DSR__M             0x1         
#define UART_GPIO_SET_DTR                0x8         /* Set GPIO 3 */
#define UART_GPIO_SET_DTR__S             3           
#define UART_GPIO_SET_DTR__M             0x1         
#define UART_GPIO_SET_CTS                0x10        /* Set GPIO 4 */
#define UART_GPIO_SET_CTS__S             4           
#define UART_GPIO_SET_CTS__M             0x1         
#define UART_GPIO_SET_RTS                0x20        /* Set GPIO 5 */
#define UART_GPIO_SET_RTS__S             5           
#define UART_GPIO_SET_RTS__M             0x1         

#define UART_GPIO_CLR                    0x0f        /* Clear GPIO output. Writing a 1 to a bit in this register drives the GPIO
                                                        output low. Writing a 0 to a bit has no effect. */
#define UART_GPIO_CLR_RI                 0x1         /* Clear GPIO 0 */
#define UART_GPIO_CLR_RI__S              0           
#define UART_GPIO_CLR_RI__M              0x1         
#define UART_GPIO_CLR_CD                 0x2         /* Clear GPIO 1 */
#define UART_GPIO_CLR_CD__S              1           
#define UART_GPIO_CLR_CD__M              0x1         
#define UART_GPIO_CLR_DSR                0x4         /* Clear GPIO 2 */
#define UART_GPIO_CLR_DSR__S             2           
#define UART_GPIO_CLR_DSR__M             0x1         
#define UART_GPIO_CLR_DTR                0x8         /* Clear GPIO 3 */
#define UART_GPIO_CLR_DTR__S             3           
#define UART_GPIO_CLR_DTR__M             0x1         
#define UART_GPIO_CLR_CTS                0x10        /* Clear GPIO 4 */
#define UART_GPIO_CLR_CTS__S             4           
#define UART_GPIO_CLR_CTS__M             0x1         
#define UART_GPIO_CLR_RTS                0x20        /* Clear GPIO 5 */
#define UART_GPIO_CLR_RTS__S             5           
#define UART_GPIO_CLR_RTS__M             0x1         

#define UART_GPIO_STATUS                 0x10        /* Returns live status of the GPIO pins. */
#define UART_GPIO_STATUS_RI              0x1         /* Status of GPIO 0 */
#define UART_GPIO_STATUS_RI__S           0           
#define UART_GPIO_STATUS_RI__M           0x1         
#define UART_GPIO_STATUS_CD              0x2         /* Status of GPIO 1 */
#define UART_GPIO_STATUS_CD__S           1           
#define UART_GPIO_STATUS_CD__M           0x1         
#define UART_GPIO_STATUS_DSR             0x4         /* Status of GPIO 2 */
#define UART_GPIO_STATUS_DSR__S          2           
#define UART_GPIO_STATUS_DSR__M          0x1         
#define UART_GPIO_STATUS_DTR             0x8         /* Status of GPIO 3 */
#define UART_GPIO_STATUS_DTR__S          3           
#define UART_GPIO_STATUS_DTR__M          0x1         
#define UART_GPIO_STATUS_CTS             0x10        /* Status of GPIO 4 */
#define UART_GPIO_STATUS_CTS__S          4           
#define UART_GPIO_STATUS_CTS__M          0x1         
#define UART_GPIO_STATUS_RTS             0x20        /* Status of GPIO 5 */
#define UART_GPIO_STATUS_RTS__S          5           
#define UART_GPIO_STATUS_RTS__M          0x1         

#define UART_GPIO_INT_MASK               0x11        
#define UART_GPIO_INT_MASK_RI            0x1         /* Dictates whether a change in GPIO 0's (RI) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_RI__S         0           
#define UART_GPIO_INT_MASK_RI__M         0x1         
#define UART_GPIO_INT_MASK_CD            0x2         /* Dictates whether a change in GPIO 1's (CD) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_CD__S         1           
#define UART_GPIO_INT_MASK_CD__M         0x1         
#define UART_GPIO_INT_MASK_DSR           0x4         /* Dictates whether a change in GPIO 2's (DSR) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_DSR__S        2           
#define UART_GPIO_INT_MASK_DSR__M        0x1         
#define UART_GPIO_INT_MASK_DTR           0x8         /* Dictates whether a change in GPIO 3's (DTR) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_DTR__S        3           
#define UART_GPIO_INT_MASK_DTR__M        0x1         
#define UART_GPIO_INT_MASK_CTS           0x10        /* Dictates whether a change in GPIO 4's (CTS) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_CTS__S        4           
#define UART_GPIO_INT_MASK_CTS__M        0x1         
#define UART_GPIO_INT_MASK_RTS           0x20        /* Dictates whether a change in GPIO 5's (RTS) pin state causes the device
                                                        to generate a USB interrupt packet. 0 means a change in the pin's state
                                                        causes the device to generate an interrupt packet. 1 means any changes in
                                                        the pin's state does not cause an interrupt packet being formed. The GPIO
                                                        status register will still report the pin's state when read, and if an
                                                        interrupt packet is formed due to other interrupt trigger, the interrupt
                                                        packet will contain the current state of the pin. */
#define UART_GPIO_INT_MASK_RTS__S        5           
#define UART_GPIO_INT_MASK_RTS__M        0x1         

#define UART_CUSTOMIZED_INT              0x12        /* Enables the customized interrupt packet format to report all GPIO status
                                                        in the interrupt packet. */
#define UART_CUSTOMIZED_INT_EN           0x1         /* 0 - use standard interrupt packet. 1 - use customized interrupt packet.
                                                        */
#define UART_CUSTOMIZED_INT_EN__S        0           
#define UART_CUSTOMIZED_INT_EN__M        0x1         

#define UART_VIO_GOOD                    0x13        
#define UART_VIO_GOOD_VAL                0x1         /* Status bit that indicates if VIO reference is currently detected. 0: VIO
                                                        REF is too low to be usable; 1: VIO REF is good for normal operation. */
#define UART_VIO_GOOD_VAL__S             0           
#define UART_VIO_GOOD_VAL__M             0x1         

#define UART_PIN_PULLUP_EN               0x14        /* Enables pad pullup feature on selected GPIO or UART Tx/Rx pins. 1=enable
                                                        pullup on the corrsponding pin. 0=disable pullup. NOTE: pullup and pulldown
                                                        should not be enabled for the same pin at the same time. */
#define UART_PIN_PULLUP_EN_RI            0x1         /* Whether to enable pad pullup on the GPIO 0 pin */
#define UART_PIN_PULLUP_EN_RI__S         0           
#define UART_PIN_PULLUP_EN_RI__M         0x1         
#define UART_PIN_PULLUP_EN_CD            0x2         /* Whether to enable pad pullup on the GPIO 1 pin */
#define UART_PIN_PULLUP_EN_CD__S         1           
#define UART_PIN_PULLUP_EN_CD__M         0x1         
#define UART_PIN_PULLUP_EN_DSR           0x4         /* Whether to enable pad pullup on the GPIO 2 pin */
#define UART_PIN_PULLUP_EN_DSR__S        2           
#define UART_PIN_PULLUP_EN_DSR__M        0x1         
#define UART_PIN_PULLUP_EN_DTR           0x8         /* Whether to enable pad pullup on the GPIO 3 pin */
#define UART_PIN_PULLUP_EN_DTR__S        3           
#define UART_PIN_PULLUP_EN_DTR__M        0x1         
#define UART_PIN_PULLUP_EN_CTS           0x10        /* Whether to enable pad pullup on the GPIO 4 pin */
#define UART_PIN_PULLUP_EN_CTS__S        4           
#define UART_PIN_PULLUP_EN_CTS__M        0x1         
#define UART_PIN_PULLUP_EN_RTS           0x20        /* Whether to enable pad pullup on the GPIO 5 pin */
#define UART_PIN_PULLUP_EN_RTS__S        5           
#define UART_PIN_PULLUP_EN_RTS__M        0x1         
#define UART_PIN_PULLUP_EN_RX            0x40        /* Whether to enable pad pullup on the UART Rx pin */
#define UART_PIN_PULLUP_EN_RX__S         6           
#define UART_PIN_PULLUP_EN_RX__M         0x1         
#define UART_PIN_PULLUP_EN_TX            0x80        /* Whether to enable pad pullup on the UART Tx pin */
#define UART_PIN_PULLUP_EN_TX__S         7           
#define UART_PIN_PULLUP_EN_TX__M         0x1         

#define UART_PIN_PULLDOWN_EN             0x15        /* Enables pad pulldown feature on selected GPIO or UART Tx/Rx pins.
                                                        1=enable pulldown on the corrsponding pin. 0=disable pulldown. NOTE: pullup
                                                        and pulldown should not be enabled for the same pin at the same time. */
#define UART_PIN_PULLDOWN_EN_RI          0x1         /* Whether to enable pad pulldown on the GPIO 0 pin */
#define UART_PIN_PULLDOWN_EN_RI__S       0           
#define UART_PIN_PULLDOWN_EN_RI__M       0x1         
#define UART_PIN_PULLDOWN_EN_CD          0x2         /* Whether to enable pad pulldown on the GPIO 1 pin */
#define UART_PIN_PULLDOWN_EN_CD__S       1           
#define UART_PIN_PULLDOWN_EN_CD__M       0x1         
#define UART_PIN_PULLDOWN_EN_DSR         0x4         /* Whether to enable pad pulldown on the GPIO 2 pin */
#define UART_PIN_PULLDOWN_EN_DSR__S      2           
#define UART_PIN_PULLDOWN_EN_DSR__M      0x1         
#define UART_PIN_PULLDOWN_EN_DTR         0x8         /* Whether to enable pad pulldown on the GPIO 3 pin */
#define UART_PIN_PULLDOWN_EN_DTR__S      3           
#define UART_PIN_PULLDOWN_EN_DTR__M      0x1         
#define UART_PIN_PULLDOWN_EN_CTS         0x10        /* Whether to enable pad pulldown on the GPIO 4 pin */
#define UART_PIN_PULLDOWN_EN_CTS__S      4           
#define UART_PIN_PULLDOWN_EN_CTS__M      0x1         
#define UART_PIN_PULLDOWN_EN_RTS         0x20        /* Whether to enable pad pulldown on the GPIO 5 pin */
#define UART_PIN_PULLDOWN_EN_RTS__S      5           
#define UART_PIN_PULLDOWN_EN_RTS__M      0x1         
#define UART_PIN_PULLDOWN_EN_RX          0x40        /* Whether to enable pad pulldown on the UART Rx pin */
#define UART_PIN_PULLDOWN_EN_RX__S       6           
#define UART_PIN_PULLDOWN_EN_RX__M       0x1         
#define UART_PIN_PULLDOWN_EN_TX          0x80        /* Whether to enable pad pulldown on the UART Tx pin */
#define UART_PIN_PULLDOWN_EN_TX__S       7           
#define UART_PIN_PULLDOWN_EN_TX__M       0x1         

#define UART_LOOPBACK                    0x16        /* Controls which UART signals are looped back. */
#define UART_LOOPBACK_TX_RX              0x1         /* When this bit is set all transmitted UART data is looped back to the
                                                        UART receiver. */
#define UART_LOOPBACK_TX_RX__S           0           
#define UART_LOOPBACK_TX_RX__M           0x1         
#define UART_LOOPBACK_RTS_CTS            0x2         /* When this bit is set RTS is looped back to CTS. */
#define UART_LOOPBACK_RTS_CTS__S         1           
#define UART_LOOPBACK_RTS_CTS__M         0x1         
#define UART_LOOPBACK_DTR_DSR            0x4         /* When this bit is set DTR is looped back to DSR. */
#define UART_LOOPBACK_DTR_DSR__S         2           
#define UART_LOOPBACK_DTR_DSR__M         0x1         

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* CBM                                                                                                                            */
/* Control Buffer Manager registers. */
/*--------------------------------------------------------------------------------------------------------------------------------*/

#define CBM_SETUP_CMD                    0x00        /* Informs PRM that a new SETUP transaction has been received and is ready
                                                        to be processed. It is also used to coordinate processing of subsequent
                                                        Control data and status stages. This register is polled by PRM to discover
                                                        that a new SETUP packet is ready for processing, and to make sure that the
                                                        data was not modified while the PRM copies it to its private working area.
                                                        */
#define CBM_SETUP_CMD_SEQ                0x7ff       /* Upon receiving a SETUP packet, CBM increments the transfer sequence
                                                        number in this field and clears the VALID field. CBM then writes the 8
                                                        bytes of received SETUP packet data into the CONTROL_SETUP buffer. If the
                                                        data is received without error CBM then sets the VALID field. */
#define CBM_SETUP_CMD_SEQ__S             0           
#define CBM_SETUP_CMD_SEQ__M             0x7ff       
#define CBM_SETUP_CMD_VALID              0x800       /* Cleared when a new SETUP packet is received, and then set once all the
                                                        SETUP packet data has been received without error and written to
                                                        CONTROL_SETUP. */
#define CBM_SETUP_CMD_VALID__S           11          
#define CBM_SETUP_CMD_VALID__M           0x1         

#define CBM_CONTROL_OUT_CMD              0x01        /* Governs the processing of the data stage transactions in a Control Write
                                                        transfer, and the status stage of a Control read transfer. It is written by
                                                        both CBM and PRM. */
#define CBM_CONTROL_OUT_CMD_SEQ          0x7ff       /* The sequence number for the current Control transfer is written by CBM
                                                        after it has successfully copied the packet data to the CONTROL_OUT buffer.
                                                        */
#define CBM_CONTROL_OUT_CMD_SEQ__S       0           
#define CBM_CONTROL_OUT_CMD_SEQ__M       0x7ff       
#define CBM_CONTROL_OUT_CMD_VALID        0x800       /* Set by CBM after it has successfully copied packet data to the
                                                        CONTROL_OUT buffer. Cleared by PRM after it has finished processing the
                                                        packet data. */
#define CBM_CONTROL_OUT_CMD_VALID__S     11          
#define CBM_CONTROL_OUT_CMD_VALID__M     0x1         

#define CBM_CONTROL_OUT_DATA             0x02        /* The number of bytes of valid data in the CONTROL_OUT buffer. Written by
                                                        CBM. */
#define CBM_CONTROL_OUT_DATA_SIZE        0x7f        /* The number of bytes of valid data in the CONTROL_OUT buffer */
#define CBM_CONTROL_OUT_DATA_SIZE__S     0           
#define CBM_CONTROL_OUT_DATA_SIZE__M     0x7f        

#define CBM_CONTROL_IN_CMD               0x03        /* Governs the processing of the data stage transactions in a Control Read
                                                        transfer, and the status stage in a Control Write transfer. It is written
                                                        by both CBM and PRM. */
#define CBM_CONTROL_IN_CMD_SEQ           0x7ff       /* After PRM has prepared the data in the CONTROL_IN buffer and set
                                                        CONTROL_IN_LEN to the number of byes of data, it writes the current Control
                                                        transfer sequence number to this field. */
#define CBM_CONTROL_IN_CMD_SEQ__S        0           
#define CBM_CONTROL_IN_CMD_SEQ__M        0x7ff       
#define CBM_CONTROL_IN_CMD_VALID         0x800       /* Set by PRM when it writes the Control transfer sequence number to SEQ.
                                                        Cleared by CBM after it receives an ACK from the host or a new SETUP token
                                                        arrive instead of a CONTROL_IN transfer. */
#define CBM_CONTROL_IN_CMD_VALID__S      11          
#define CBM_CONTROL_IN_CMD_VALID__M      0x1         

#define CBM_CONTROL_IN_DATA              0x04        /* This register governs the response to a control IN transaction. When PRM
                                                        detects a protocol error it sets the ERROR flag to indicate that a STALL is
                                                        sent. Otherwise the The number of bytes of valid data in the CONTROL_IN
                                                        buffer. Written by PRM. */
#define CBM_CONTROL_IN_DATA_SIZE         0x7f        /* The number of bytes to send as a successful response to a control IN
                                                        token. */
#define CBM_CONTROL_IN_DATA_SIZE__S      0           
#define CBM_CONTROL_IN_DATA_SIZE__M      0x7f        
#define CBM_CONTROL_IN_DATA_ERROR        0x80        /* When this bit is set we send a STALL instead of data. This is set by PRM
                                                        when it signals a protocol error. */
#define CBM_CONTROL_IN_DATA_ERROR__S     7           
#define CBM_CONTROL_IN_DATA_ERROR__M     0x1         

#define CBM_USB_CONTROL                  0x05        /* This bit was homeless and ended up here. */
#define CBM_USB_CONTROL_EN_REMOTE_WAKE   0x1         /* If 1 the host has enabled remote wakeup. If 0 then remote wakeup is
                                                        disabled. */
#define CBM_USB_CONTROL_EN_REMOTE_WAKE__S 0           
#define CBM_USB_CONTROL_EN_REMOTE_WAKE__M 0x1         

#define CBM_BIST_STATUS                  0x06        /* Provides status of ROM and RAM BIST. ROM status can only be read by PRM.
                                                        PRM performs RAM BIST and writes the results here. All bits can be read
                                                        over USB. */
#define CBM_BIST_STATUS_PRM_BIST_DONE    0x1         
#define CBM_BIST_STATUS_PRM_BIST_DONE__S 0           
#define CBM_BIST_STATUS_PRM_BIST_DONE__M 0x1         
#define CBM_BIST_STATUS_PRM_BIST_PASS    0x2         
#define CBM_BIST_STATUS_PRM_BIST_PASS__S 1           
#define CBM_BIST_STATUS_PRM_BIST_PASS__M 0x1         
#define CBM_BIST_STATUS_CTL_BIST_DONE    0x4         
#define CBM_BIST_STATUS_CTL_BIST_DONE__S 2           
#define CBM_BIST_STATUS_CTL_BIST_DONE__M 0x1         
#define CBM_BIST_STATUS_CTL_BIST_PASS    0x8         
#define CBM_BIST_STATUS_CTL_BIST_PASS__S 3           
#define CBM_BIST_STATUS_CTL_BIST_PASS__M 0x1         
#define CBM_BIST_STATUS_CMA_BIST_DONE    0x10        
#define CBM_BIST_STATUS_CMA_BIST_DONE__S 4           
#define CBM_BIST_STATUS_CMA_BIST_DONE__M 0x1         
#define CBM_BIST_STATUS_CMA_BIST_PASS    0x20        
#define CBM_BIST_STATUS_CMA_BIST_PASS__S 5           
#define CBM_BIST_STATUS_CMA_BIST_PASS__M 0x1         
#define CBM_BIST_STATUS_RAM_BIST_DONE    0x40        /* This bit is set by the PRM when the internal RAM test is done. The PRM
                                                        has full r/w access and the default value is 0. */
#define CBM_BIST_STATUS_RAM_BIST_DONE__S 6           
#define CBM_BIST_STATUS_RAM_BIST_DONE__M 0x1         
#define CBM_BIST_STATUS_RAM_BIST_PASS    0x80        /* This bit is set by the PRM when the internal RAM test PASSED. The PRM
                                                        has full r/w access and the default value is 0. */
#define CBM_BIST_STATUS_RAM_BIST_PASS__S 7           
#define CBM_BIST_STATUS_RAM_BIST_PASS__M 0x1         
#define CBM_BIST_STATUS_OTP_READY        0x100       /* This bit is set by the PRM during startup to indicate whether the OTP is
                                                        functional. It will be set to 1 when (OTP_STATUS_BUSY = 0 and
                                                        OTP_STATUS_PWR_UP = 1). If the check failed the first time, the PRM will
                                                        wait for 10us and check again before confirming that the OTP is not
                                                        functional. The PRM has full r/w access and the default value is 0. */
#define CBM_BIST_STATUS_OTP_READY__S     8           
#define CBM_BIST_STATUS_OTP_READY__M     0x1         

#define CBM_SHAG_ENABLE                  0x07        
#define CBM_SHAG_ENABLE_EN               0x1         /* This bit defaults to 0 which prevents setting any of the shags thus
                                                        allowing the PRM to perform the shag test. It must be set to 1 (by PRM) for
                                                        normal operation. */
#define CBM_SHAG_ENABLE_EN__S            0           
#define CBM_SHAG_ENABLE_EN__M            0x1         

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* BFMTX                                                                                                                          */
/* Bulk TX FIFO Manager registers. */
/*--------------------------------------------------------------------------------------------------------------------------------*/

#define BFMTX_RESET                      0x00        /* Reset the TX FIFO to empty. */
#define BFMTX_RESET_FIFO                 0x1         /* Write 1 to reset the TX FIFO to empty. Always reads back as 0. */
#define BFMTX_RESET_FIFO__S              0           
#define BFMTX_RESET_FIFO__M              0x1         

#define BFMTX_COUNT                      0x01        /* The number of characters currently in the TX FIFO. */
#define BFMTX_COUNT_FILL                 0xff        /* The number of characters currently in the TX FIFO. */
#define BFMTX_COUNT_FILL__S              0           
#define BFMTX_COUNT_FILL__M              0xff        

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* BFMRX                                                                                                                          */
/* Bulk RX FIFO Manager registers. */
/*--------------------------------------------------------------------------------------------------------------------------------*/

#define BFMRX_RESET                      0x00        /* Reset the RX FIFO to empty. */
#define BFMRX_RESET_FIFO                 0x1         /* Write 1 to reset the RX FIFO to empty. Always reads back as 0. */
#define BFMRX_RESET_FIFO__S              0           
#define BFMRX_RESET_FIFO__M              0x1         

#define BFMRX_COUNT                      0x01        /* The number of characters currently in the RX FIFO. */
#define BFMRX_COUNT_FILL                 0x1ff       /* The number of characters currently in the RX FIFO. */
#define BFMRX_COUNT_FILL__S              0           
#define BFMRX_COUNT_FILL__M              0x1ff       

#define BFMRX_LOW_LATENCY                0x02        /* In normal operation all bulk-in transfer will be maxPacketSize (64)
                                                        bytes to improve throughput and to minimize host processing. However, in
                                                        cases where the baud rate is low this increases latency unacceptably. This
                                                        bit is set by PRM (and possibly by the host driver) to force RX data to be
                                                        delivered without delay. In the normal case (LOW_LATENCY is clear), BFMRX
                                                        should hold on to the data until either: it is stale; or the RX FIFO has
                                                        more than maxPacketSize data available (i.e. 32 characters in wide mode, 64
                                                        characters in normal mode). */
#define BFMRX_LOW_LATENCY_LOW_LATENCY_EN 0x1         /* If set disables the stale heuristic, causing data to be sent without
                                                        delay. */
#define BFMRX_LOW_LATENCY_LOW_LATENCY_EN__S 0           
#define BFMRX_LOW_LATENCY_LOW_LATENCY_EN__M 0x1         

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* EPM                                                                                                                            */
/* Endpoint Manager registers. EP0 is used for all Control transfers. EP1 is used for bulk IN and OUT transactions. EP5 is used
   for Interrupt IN transactions. */
/*--------------------------------------------------------------------------------------------------------------------------------*/

#define EPM_CURRENT                      0x00        /* The currently configured USB device address. The EPM must ignore all
                                                        packets addressed to other device addresses. This register is updated from
                                                        NEXT *after* the current control write transfer is successfully completed.
                                                        One way this could be implemented is for NEXT to be copied to CURRENT
                                                        whenever an ACK for a ZLP is received. */
#define EPM_CURRENT_ADDR                 0x7f        /* The current device address. */
#define EPM_CURRENT_ADDR__S              0           
#define EPM_CURRENT_ADDR__M              0x7f        

#define EPM_NEXT                         0x01        /* This register is written by PRM when it processes a SET_ADDRESS control
                                                        message. It is copied into CURRENT *after* the current control write
                                                        transfer is successfully completed. In other words, the status stage of the
                                                        transfer continues to use the old address, and the new address doesn't take
                                                        effect until after our ZLP is ACKed by the host. */
#define EPM_NEXT_ADDR                    0x7f        /* The next device address. */
#define EPM_NEXT_ADDR__S                 0           
#define EPM_NEXT_ADDR__M                 0x7f        

#define EPM_WIDE                         0x02        /* Informs EPM that bulk data is being transferred in wide mode. This means
                                                        that every pair of bytes of bulk data received from the host is merged into
                                                        a single 12-bit word and written to the BFM. Conversely, every 12-bit word
                                                        read from the BFM is converted into a pair of bytes and transferred to the
                                                        host. When in wide mode the host guarantees to always send an even number
                                                        of bytes in each transaction. */
#define EPM_WIDE_EN                      0x1         /* Set to wide mode; clear to use normal mode. */
#define EPM_WIDE_EN__S                   0           
#define EPM_WIDE_EN__M                   0x1         

#define EPM_BULK_IN_MAX_LENGTH           0x03        /* An internal register used to remember the length of a bulk IN
                                                        transaction so that it can be retransmitted accurately. Exposed here for
                                                        debugging only. */
#define EPM_BULK_IN_MAX_LENGTH_RESETVAL  0x040       

#define EPM_DEV_STATE                    0x04        /* Indicates which USB state the device is currently in. If it is not
                                                        CONFIGURED then EPM must ignore all transactions except those to EP0. The
                                                        ADDRESS bit can be ignored by the EPM; it is included in this register
                                                        rather than in PRM memory to make the PRM code simpler. The DEFAULT STATE
                                                        is when the register is zero. */
#define EPM_DEV_STATE_ADDRESS            0x1         /* Set if the device is in the USB addressed state. Only written and read
                                                        by PRM. */
#define EPM_DEV_STATE_ADDRESS__S         0           
#define EPM_DEV_STATE_ADDRESS__M         0x1         
#define EPM_DEV_STATE_CONFIGURED         0x2         /* Set if the device is in the USB configured state. Written and read by
                                                        PRM, and read by EPM. If this bit is clear EPM must ignore all transactions
                                                        to endpoints other than EP0. */
#define EPM_DEV_STATE_CONFIGURED__S      1           
#define EPM_DEV_STATE_CONFIGURED__M      0x1         

#define EPM_EP_HALT                      0x05        /* Endpoint 0 is never halted, but other endpoints sometimes are. They can
                                                        be explicitly halted and unhalted by the host with a SET_HALT or CLEAR_HALT
                                                        control message. If EPM receives a packet to a halted endpoint it responds
                                                        with a STALL handshake. Transactions to non-existent endpoints are simply
                                                        ignored by EPM. */
#define EPM_EP_HALT_BULK_IN              0x1         /* When this bit is set the EPM must return a STALL for each transaction
                                                        directed to the bulk IN endpoint. */
#define EPM_EP_HALT_BULK_IN__S           0           
#define EPM_EP_HALT_BULK_IN__M           0x1         
#define EPM_EP_HALT_BULK_OUT             0x2         /* When this bit is set the EPM must return a STALL for each transaction
                                                        directed to the bulk OUT endpoint. No data is transferred to the TX FIFO.
                                                        */
#define EPM_EP_HALT_BULK_OUT__S          1           
#define EPM_EP_HALT_BULK_OUT__M          0x1         
#define EPM_EP_HALT_INT_IN               0x4         /* When this bit is set the EPM must return a STALL for each transaction
                                                        directed to the interrupt IN endpoint. */
#define EPM_EP_HALT_INT_IN__S            2           
#define EPM_EP_HALT_INT_IN__M            0x1         

#define EPM_ENABLE                       0x06        /* This register determines the operation of the EPM. */
#define EPM_ENABLE_EN                    0x1         /* If this bit is set to 1, the EPM will be enabled. When set to 0, the EPM
                                                        will be disabled and ignore all request from the host. */
#define EPM_ENABLE_EN__S                 0           
#define EPM_ENABLE_EN__M                 0x1         

#define EPM_USB_PWR                      0x07        /* This register controls the power options to the digital phy. */
#define EPM_USB_PWR_LP                   0x1         /* When set, the digital phy will enter to a low power mode. The default
                                                        mode is 0. PRM has r/w access to this register. */
#define EPM_USB_PWR_LP__S                0           
#define EPM_USB_PWR_LP__M                0x1         
#define EPM_USB_PWR_SC                   0x2         /* When set, the digital phy will enter to a slow clock mode. The default
                                                        mode is 0. PRM has r/w access to this register. */
#define EPM_USB_PWR_SC__S                1           
#define EPM_USB_PWR_SC__M                0x1         
#define EPM_USB_PWR_RESERVED             0xc         /* Reserved bit needed for EPM */
#define EPM_USB_PWR_RESERVED__S          2           
#define EPM_USB_PWR_RESERVED__M          0x3         
#define EPM_USB_PWR_XCVR                 0x10        /* When set, the USB Phy transciever will be enabled. The default mode is
                                                        0. PRM has r/w access to this register. */
#define EPM_USB_PWR_XCVR__S              4           
#define EPM_USB_PWR_XCVR__M              0x1         
#define EPM_USB_PWR_LDO                  0x20        /* When set, the USB Phy transciever will be send a LDO output voltage to
                                                        the USB PHY_DN pin. Default mode is 0. PRM has r/w access to this register.
                                                        */
#define EPM_USB_PWR_LDO__S               5           
#define EPM_USB_PWR_LDO__M               0x1         
#define EPM_USB_PWR_RESERVED1            0x40        /* Reserved bit needed for EPM */
#define EPM_USB_PWR_RESERVED1__S         6           
#define EPM_USB_PWR_RESERVED1__M         0x1         
#define EPM_USB_PWR_RSM                  0x80        /* When set, the USB Phy will be send a USB resume request to the host. The
                                                        default mode is 0. PRM has r/w access to this register. */
#define EPM_USB_PWR_RSM__S               7           
#define EPM_USB_PWR_RSM__M               0x1         

#define EPM_USB_TST                      0x08        /* This register controls the power options and test mode control to the
                                                        digital phy. */
#define EPM_USB_TST_CONTROL              0x3         /* Encoding for the test mode control. In normal operation, CONTROL is 0.
                                                        PRM has r/w access to this register. */
#define EPM_USB_TST_CONTROL__S           0           
#define EPM_USB_TST_CONTROL__M           0x3         
#define EPM_USB_TST_CONTROL_NORMAL       0x0         
#define EPM_USB_TST_CONTROL_J_STATE      0x1         
#define EPM_USB_TST_CONTROL_K_STATE      0x2         
#define EPM_USB_TST_CONTROL_SE0_STATE    0x3         
#define EPM_USB_TST_RP1                  0x4         /* When set, the USB bus will be pull up 1. The default mode is 0. PRM has
                                                        r/w access to this register. */
#define EPM_USB_TST_RP1__S               2           
#define EPM_USB_TST_RP1__M               0x1         
#define EPM_USB_TST_RP2                  0x8         /* When set, the USB bus will be pull up 2. The default mode is 0. PRM has
                                                        r/w access to this register. */
#define EPM_USB_TST_RP2__S               3           
#define EPM_USB_TST_RP2__M               0x1         

#define EPM_USB_TX_ZEROS                 0x09        /* This register controls the digital PHY test mode for sending all zeros
                                                        in the data payload. */
#define EPM_USB_TX_ZEROS_EN              0x1         /* Send continuous zeros on the USB bus when set to 1. Default value is 0.
                                                        PRM has r/w access to this register. */
#define EPM_USB_TX_ZEROS_EN__S           0           
#define EPM_USB_TX_ZEROS_EN__M           0x1         

#define EPM_USB_TX_ONES                  0x0a        /* This register controls the digital PHY test mode for sending all ones in
                                                        the data payload. */
#define EPM_USB_TX_ONES_EN               0x1         /* Send continuous ones on the USB bus when set to 1. Default value is 0.
                                                        PRM has r/w access to this register. */
#define EPM_USB_TX_ONES_EN__S            0           
#define EPM_USB_TX_ONES_EN__M            0x1         

#define EPM_SCANMODE                     0x0b        /* This register control the scan mode. */
#define EPM_SCANMODE_EN                  0x1         /* If this bit is set to 1, the scan mode will be enable. The PRM is
                                                        responsible to set this bit to 1 when the host enable this using the
                                                        SET_FEATURE command. This mode is destructive and need to do a hard reset
                                                        to exit. */
#define EPM_SCANMODE_EN__S               0           
#define EPM_SCANMODE_EN__M               0x1         

#define EPM_PHY_STATUS                   0x0c        /* This register controls the digital PHY suspend and remote wakeup
                                                        request. */
#define EPM_PHY_STATUS_SUSP_REQ          0x1         /* When set, this indicates a suspend request from the host. This usually
                                                        happen when the USB bus is idle for more than 3ms. The device can take up
                                                        to 10ms to go into susspend mode. The EPM has r/w access to this register
                                                        and PRM is read only. Default value is 0. */
#define EPM_PHY_STATUS_SUSP_REQ__S       0           
#define EPM_PHY_STATUS_SUSP_REQ__M       0x1         
#define EPM_PHY_STATUS_HOST_WAKEUP_REQ   0x2         /* When set, this indicates a remote wakup request from the host. This
                                                        usually happen when the host initiate a wake up signaling on the USB bus
                                                        for more than 3ms. The device can take up to 10ms to return to normal mode.
                                                        The EPM has r/w access to this register and PRM is read only. Default value
                                                        is 0. */
#define EPM_PHY_STATUS_HOST_WAKEUP_REQ__S 1           
#define EPM_PHY_STATUS_HOST_WAKEUP_REQ__M 0x1         
#define EPM_PHY_STATUS_DEV_WAKEUP_REQ    0x4         /* When set, this indicates a remote wakup request by the device. This
                                                        usually happen when the device recieived wake up request on RI input. The
                                                        device will send resume signaling on the USB bus to the host. The EPM has
                                                        r/w access to this register and PRM is read only. Default value is 0. */
#define EPM_PHY_STATUS_DEV_WAKEUP_REQ__S 2           
#define EPM_PHY_STATUS_DEV_WAKEUP_REQ__M 0x1         
#define EPM_PHY_STATUS_SOF               0x8         /* When this bit changes from 0 to 1 or 1 to 0, this indicates an SOF is
                                                        successfully detected. This flag is used in TEST MODE */
#define EPM_PHY_STATUS_SOF__S            3           
#define EPM_PHY_STATUS_SOF__M            0x1         

/*--------------------------------------------------------------------------------------------------------------------------------*/
/* OTP                                                                                                                            */

/*--------------------------------------------------------------------------------------------------------------------------------*/

#define OTP_REVISION_ID                  0x00        /* The silicon revision ID of Mongo. This value is returned verbatim in the
                                                        bcdDevice field of the device descriptor. This means that its value must be
                                                        a pair of BCD digits. */
#define OTP_REVISION_ID_VAL              0xff        /* The silicon revision ID of Mongo. Rev A is 0. */
#define OTP_REVISION_ID_VAL__S           0           
#define OTP_REVISION_ID_VAL__M           0xff        

#define OTP_WRITE_DATA                   0x01        /* The 8-bit value to be written to OTP main memory. Also used as the LSB
                                                        [7:0] of the mode register write data. */
#define OTP_WRITE_DATA_VAL               0xff        
#define OTP_WRITE_DATA_VAL__S            0           
#define OTP_WRITE_DATA_VAL__M            0xff        

#define OTP_INDIRECT                     0x02        /* The byte address to read from or write to. */
#define OTP_INDIRECT_ADDR                0x7ff       /* Address into main memory. If bit[10] is 1 then the 16-byte boot space is
                                                        being accessed and only bits [3:0] are valid. For main memory, the OTP's
                                                        redundant read mode is used. Also, otp_ctrl will remap this address such
                                                        that address 0-255 are useable and default to 0x00. 256-511 are useable and
                                                        default to 0xff. 512-767 are redundant copies of 0-255. 768-1023 are
                                                        redundant copies of 256-511. */
#define OTP_INDIRECT_ADDR__S             0           
#define OTP_INDIRECT_ADDR__M             0x7ff       

#define OTP_CONTROL                      0x03        /* Control operations for reading, writing, and verifying OTP. The BUSY
                                                        flag is set immediately and is cleared when the operation is complete. */
#define OTP_CONTROL_CMD                  0xf         /* Encoding for the requested operation. This field is cleared once the
                                                        command is complete. Notes: WRITE only sets the input data latch and must
                                                        be followed by PROGRAM. PWR_UP requires 3us recovery time to complete.
                                                        PROGRAM takes 50us to complete with default pulse width. */
#define OTP_CONTROL_CMD__S               0           
#define OTP_CONTROL_CMD__M               0xf         
#define OTP_CONTROL_CMD_IDLE             0x0         
#define OTP_CONTROL_CMD_READ             0x1         
#define OTP_CONTROL_CMD_PRECHARGE        0x2         
#define OTP_CONTROL_CMD_WRITE            0x4         
#define OTP_CONTROL_CMD_WRITE_MR         0x5         
#define OTP_CONTROL_CMD_AUX_A            0x6         
#define OTP_CONTROL_CMD_AUX_B            0x7         
#define OTP_CONTROL_CMD_PROGRAM          0x8         
#define OTP_CONTROL_CMD_PWR_DOWN         0x9         
#define OTP_CONTROL_CMD_PWR_UP           0xa         

#define OTP_STATUS                       0x04        /* Status outputs from the OTP and the control logic. */
#define OTP_STATUS_BUSY                  0x1         /* Automatically set as soon as an OTP operation is started, and cleared
                                                        when the operation is completed. The PRM sets the command bit and then
                                                        immediately starts polling this flag and waits for it to be clear. */
#define OTP_STATUS_BUSY__S               0           
#define OTP_STATUS_BUSY__M               0x1         
#define OTP_STATUS_PWR_UP                0x2         /* Power-up reset output from OTP. HIGH when power detected. */
#define OTP_STATUS_PWR_UP__S             1           
#define OTP_STATUS_PWR_UP__M             0x1         
#define OTP_STATUS_VPP_MON               0x4         /* If enabled (HIGH), indicates that VPP is applied. */
#define OTP_STATUS_VPP_MON__S            2           
#define OTP_STATUS_VPP_MON__M            0x1         
#define OTP_STATUS_STATUS                0x8         /* Comparator Output from the OTP. Active high. */
#define OTP_STATUS_STATUS__S             3           
#define OTP_STATUS_STATUS__M             0x1         
#define OTP_STATUS_STATUS_LATCH          0x10        /* Sticky version of the STATUS bit. It is cleared at the start of every
                                                        READ, PRECHARGE, or COMPARE command. */
#define OTP_STATUS_STATUS_LATCH__S       4           
#define OTP_STATUS_STATUS_LATCH__M       0x1         

#define OTP_READ_DATA                    0x05        /* The 8-bit value read from OTP. */
#define OTP_READ_DATA_VAL                0xff        
#define OTP_READ_DATA_VAL__S             0           
#define OTP_READ_DATA_VAL__M             0xff        

#define OTP_WRITE_DATA_MR_MSB            0x06        /* The value to be written to the MSB bits [15:8] of the OTP Mode Register.
                                                        The LSB bits [7:0] come from the WRITE_DATA register. */
#define OTP_WRITE_DATA_MR_MSB_VAL        0xff        
#define OTP_WRITE_DATA_MR_MSB_VAL__S     0           
#define OTP_WRITE_DATA_MR_MSB_VAL__M     0xff        

#define OTP_READ_DATA_MR_LSB             0x07        /* The LSB bits [7:0] of the value read from the OTP Mode Register. */
#define OTP_READ_DATA_MR_LSB_VAL         0xff        
#define OTP_READ_DATA_MR_LSB_VAL__S      0           
#define OTP_READ_DATA_MR_LSB_VAL__M      0xff        

#define OTP_READ_DATA_MR_MSB             0x08        /* The MSB bits [15:8] of the value read from the OTP Mode Register. */
#define OTP_READ_DATA_MR_MSB_VAL         0xff        
#define OTP_READ_DATA_MR_MSB_VAL__S      0           
#define OTP_READ_DATA_MR_MSB_VAL__M      0xff        

#define OTP_READ_DATA_MRA_LSB            0x09        /* The LSB bits [7:0] of the value read from the OTP Mode Auxiliary A
                                                        Register. */
#define OTP_READ_DATA_MRA_LSB_VAL        0xff        
#define OTP_READ_DATA_MRA_LSB_VAL__S     0           
#define OTP_READ_DATA_MRA_LSB_VAL__M     0xff        

#define OTP_READ_DATA_MRA_MSB            0x0a        /* The MSB bits [15:8] of the value read from the OTP Mode Auxiliary A
                                                        Register. */
#define OTP_READ_DATA_MRA_MSB_VAL        0xff        
#define OTP_READ_DATA_MRA_MSB_VAL__S     0           
#define OTP_READ_DATA_MRA_MSB_VAL__M     0xff        

#define OTP_READ_DATA_MRB_LSB            0x0b        /* The LSB bits [7:0] of the value read from the OTP Mode Auxiliary B
                                                        Register. */
#define OTP_READ_DATA_MRB_LSB_VAL        0xff        
#define OTP_READ_DATA_MRB_LSB_VAL__S     0           
#define OTP_READ_DATA_MRB_LSB_VAL__M     0xff        

#define OTP_READ_DATA_MRB_MSB            0x0c        /* The MSB bits [15:8] of the value read from the OTP Mode Auxiliary B
                                                        Register. */
#define OTP_READ_DATA_MRB_MSB_VAL        0xff        
#define OTP_READ_DATA_MRB_MSB_VAL__S     0           
#define OTP_READ_DATA_MRB_MSB_VAL__M     0xff        

#define OTP_CHARGE_PUMP_CONTROL          0x0d        /* Control the 5V switch which supplies the charge pump. */
#define OTP_CHARGE_PUMP_CONTROL_VPP_EN   0x1         /* Enables the 5V switch to supply the input charge pump voltage. Must be
                                                        enabled several microseconds before turning on the charge pump through MRA.
                                                        */
#define OTP_CHARGE_PUMP_CONTROL_VPP_EN__S 0           
#define OTP_CHARGE_PUMP_CONTROL_VPP_EN__M 0x1         

#define OTP_PGM_PULSE_CONTROL            0x0e        /* Allows adjustment of the program pulse width. */
#define OTP_PGM_PULSE_CONTROL_WIDTH      0x7f        /* Specifies the width of the PGM pulse for PROGRAM commands in increments
                                                        of 10us (nominal). A value of 0 is invalid. This allows pulse widths of
                                                        10us to 1,270us. */
#define OTP_PGM_PULSE_CONTROL_WIDTH__S   0           
#define OTP_PGM_PULSE_CONTROL_WIDTH__M   0x7f        
#define OTP_PGM_PULSE_CONTROL_INFINITE   0x80        /* The PGM pin is immediately set active and will remain so as long as this
                                                        bit is set. */
#define OTP_PGM_PULSE_CONTROL_INFINITE__S 7           
#define OTP_PGM_PULSE_CONTROL_INFINITE__M 0x1         
#define OTP_PGM_PULSE_CONTROL_RESETVAL   0x005       

#define OTP_READ_PULSE_CONTROL           0x0f        /* Allows adjustment of the read pulse width. */
#define OTP_READ_PULSE_CONTROL_WIDTH     0x7f        /* Specifies the width of the READ pulse in increments of 5ns (nominal). A
                                                        value of 0 is invalid. This allows pulse widths of 5ns to 635ns. */
#define OTP_READ_PULSE_CONTROL_WIDTH__S  0           
#define OTP_READ_PULSE_CONTROL_WIDTH__M  0x7f        
#define OTP_READ_PULSE_CONTROL_RESETVAL  0x00d       

#define OTP_TEST_OSC_TRIM_COARSE         0x10        /* This register is intended for use during ATE trimming only. */
#define OTP_TEST_OSC_TRIM_COARSE_VAL     0x3f        /* Provides the coarse trim value for the internal oscillator when the OTP
                                                        is unprogrammed. */
#define OTP_TEST_OSC_TRIM_COARSE_VAL__S  0           
#define OTP_TEST_OSC_TRIM_COARSE_VAL__M  0x3f        
#define OTP_TEST_OSC_TRIM_COARSE_RESETVAL 0x020       

#define OTP_TEST_OSC_TRIM_FINE           0x11        /* This register is intended for use during ATE trimming only. */
#define OTP_TEST_OSC_TRIM_FINE_VAL       0xff        /* Provides the fine trim value for the internal osciallator when the OTP
                                                        is unprogrammed. */
#define OTP_TEST_OSC_TRIM_FINE_VAL__S    0           
#define OTP_TEST_OSC_TRIM_FINE_VAL__M    0xff        
#define OTP_TEST_OSC_TRIM_FINE_RESETVAL  0x080       

#define OTP_TEST_CLK_CTRL                0x12        /* Provides overrides of OTP bits for controlling the clock. */
#define OTP_TEST_CLK_CTRL_FORCE_SUSPEND_CLK 0x1         /* If 1, forces the clock to be output onto the SUSPEND pin even if it
                                                           has already been trimmed. If 0 and the clock is trimmed then the SUSPEND
                                                           pin performs its intended function. */
#define OTP_TEST_CLK_CTRL_FORCE_SUSPEND_CLK__S 0           
#define OTP_TEST_CLK_CTRL_FORCE_SUSPEND_CLK__M 0x1         
#define OTP_TEST_CLK_CTRL_OVERRIDE_TRIM  0x2         /* If 1, overrides the OTP trim with registers even if osc_trimmed from OTP
                                                        is 1. */
#define OTP_TEST_CLK_CTRL_OVERRIDE_TRIM__S 1           
#define OTP_TEST_CLK_CTRL_OVERRIDE_TRIM__M 0x1         
#define OTP_TEST_CLK_CTRL_VBUS_SENSE_LEVEL 0x4         /* Returns live status of VBUS_SENSE pin for ATE test when
                                                          ENABLE_VBUS_SENSE is 0. */
#define OTP_TEST_CLK_CTRL_VBUS_SENSE_LEVEL__S 2           
#define OTP_TEST_CLK_CTRL_VBUS_SENSE_LEVEL__M 0x1         

#define OTP_TEST_FEED_FWD                0x13        /* Provides a way to set FEED_FWD configuration before burning to OTP. */
#define OTP_TEST_FEED_FWD_SKEW           0xf         
#define OTP_TEST_FEED_FWD_SKEW__S        0           
#define OTP_TEST_FEED_FWD_SKEW__M        0xf         
#define OTP_TEST_FEED_FWD_CURRENT        0xf0        
#define OTP_TEST_FEED_FWD_CURRENT__S     4           
#define OTP_TEST_FEED_FWD_CURRENT__M     0xf         
#define OTP_TEST_FEED_FWD_OVERRIDE       0x100       /* If 1, FEED_FWD configuration comes from this register. Otherwise it
                                                        comes from OTP. */
#define OTP_TEST_FEED_FWD_OVERRIDE__S    8           
#define OTP_TEST_FEED_FWD_OVERRIDE__M    0x1         
#endif


#define PRM_CUSTOM_DRIVER                0x00d
#define PRM_CUSTOM_DRIVER_ACTIVE         0x1
