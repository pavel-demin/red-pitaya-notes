//SI5351 REFER FROM https://github.com/afiskon/stm32-si5351.git

#include <stdlib.h>
#include <string.h>
#include "xiicps.h"
#include "xemacps.h"
#include "xil_printf.h"

const u8 ADDR_EEPROM = 0x50;
const u8 ADDR_SI5351 = 0x60;

const u32 XREF_FREQ = 24576000;

const u32 correction = 0;

/* Si5351 driver*/

// Set of Si5351A register addresses

// See http://www.silabs.com/Support%20Documents/TechnicalDocs/AN619.pdf
enum {
    SI5351_REGISTER_0_DEVICE_STATUS                       = 0,
    SI5351_REGISTER_1_INTERRUPT_STATUS_STICKY             = 1,
    SI5351_REGISTER_2_INTERRUPT_STATUS_MASK               = 2,
    SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL               = 3,
    SI5351_REGISTER_9_OEB_PIN_ENABLE_CONTROL              = 9,
    SI5351_REGISTER_15_PLL_INPUT_SOURCE                   = 15,
    SI5351_REGISTER_16_CLK0_CONTROL                       = 16,
    SI5351_REGISTER_17_CLK1_CONTROL                       = 17,
    SI5351_REGISTER_18_CLK2_CONTROL                       = 18,
    SI5351_REGISTER_19_CLK3_CONTROL                       = 19,
    SI5351_REGISTER_20_CLK4_CONTROL                       = 20,
    SI5351_REGISTER_21_CLK5_CONTROL                       = 21,
    SI5351_REGISTER_22_CLK6_CONTROL                       = 22,
    SI5351_REGISTER_23_CLK7_CONTROL                       = 23,
    SI5351_REGISTER_24_CLK3_0_DISABLE_STATE               = 24,
    SI5351_REGISTER_25_CLK7_4_DISABLE_STATE               = 25,
    SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1           = 42,
    SI5351_REGISTER_43_MULTISYNTH0_PARAMETERS_2           = 43,
    SI5351_REGISTER_44_MULTISYNTH0_PARAMETERS_3           = 44,
    SI5351_REGISTER_45_MULTISYNTH0_PARAMETERS_4           = 45,
    SI5351_REGISTER_46_MULTISYNTH0_PARAMETERS_5           = 46,
    SI5351_REGISTER_47_MULTISYNTH0_PARAMETERS_6           = 47,
    SI5351_REGISTER_48_MULTISYNTH0_PARAMETERS_7           = 48,
    SI5351_REGISTER_49_MULTISYNTH0_PARAMETERS_8           = 49,
    SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1           = 50,
    SI5351_REGISTER_51_MULTISYNTH1_PARAMETERS_2           = 51,
    SI5351_REGISTER_52_MULTISYNTH1_PARAMETERS_3           = 52,
    SI5351_REGISTER_53_MULTISYNTH1_PARAMETERS_4           = 53,
    SI5351_REGISTER_54_MULTISYNTH1_PARAMETERS_5           = 54,
    SI5351_REGISTER_55_MULTISYNTH1_PARAMETERS_6           = 55,
    SI5351_REGISTER_56_MULTISYNTH1_PARAMETERS_7           = 56,
    SI5351_REGISTER_57_MULTISYNTH1_PARAMETERS_8           = 57,
    SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1           = 58,
    SI5351_REGISTER_59_MULTISYNTH2_PARAMETERS_2           = 59,
    SI5351_REGISTER_60_MULTISYNTH2_PARAMETERS_3           = 60,
    SI5351_REGISTER_61_MULTISYNTH2_PARAMETERS_4           = 61,
    SI5351_REGISTER_62_MULTISYNTH2_PARAMETERS_5           = 62,
    SI5351_REGISTER_63_MULTISYNTH2_PARAMETERS_6           = 63,
    SI5351_REGISTER_64_MULTISYNTH2_PARAMETERS_7           = 64,
    SI5351_REGISTER_65_MULTISYNTH2_PARAMETERS_8           = 65,
    SI5351_REGISTER_66_MULTISYNTH3_PARAMETERS_1           = 66,
    SI5351_REGISTER_67_MULTISYNTH3_PARAMETERS_2           = 67,
    SI5351_REGISTER_68_MULTISYNTH3_PARAMETERS_3           = 68,
    SI5351_REGISTER_69_MULTISYNTH3_PARAMETERS_4           = 69,
    SI5351_REGISTER_70_MULTISYNTH3_PARAMETERS_5           = 70,
    SI5351_REGISTER_71_MULTISYNTH3_PARAMETERS_6           = 71,
    SI5351_REGISTER_72_MULTISYNTH3_PARAMETERS_7           = 72,
    SI5351_REGISTER_73_MULTISYNTH3_PARAMETERS_8           = 73,
    SI5351_REGISTER_74_MULTISYNTH4_PARAMETERS_1           = 74,
    SI5351_REGISTER_75_MULTISYNTH4_PARAMETERS_2           = 75,
    SI5351_REGISTER_76_MULTISYNTH4_PARAMETERS_3           = 76,
    SI5351_REGISTER_77_MULTISYNTH4_PARAMETERS_4           = 77,
    SI5351_REGISTER_78_MULTISYNTH4_PARAMETERS_5           = 78,
    SI5351_REGISTER_79_MULTISYNTH4_PARAMETERS_6           = 79,
    SI5351_REGISTER_80_MULTISYNTH4_PARAMETERS_7           = 80,
    SI5351_REGISTER_81_MULTISYNTH4_PARAMETERS_8           = 81,
    SI5351_REGISTER_82_MULTISYNTH5_PARAMETERS_1           = 82,
    SI5351_REGISTER_83_MULTISYNTH5_PARAMETERS_2           = 83,
    SI5351_REGISTER_84_MULTISYNTH5_PARAMETERS_3           = 84,
    SI5351_REGISTER_85_MULTISYNTH5_PARAMETERS_4           = 85,
    SI5351_REGISTER_86_MULTISYNTH5_PARAMETERS_5           = 86,
    SI5351_REGISTER_87_MULTISYNTH5_PARAMETERS_6           = 87,
    SI5351_REGISTER_88_MULTISYNTH5_PARAMETERS_7           = 88,
    SI5351_REGISTER_89_MULTISYNTH5_PARAMETERS_8           = 89,
    SI5351_REGISTER_90_MULTISYNTH6_PARAMETERS             = 90,
    SI5351_REGISTER_91_MULTISYNTH7_PARAMETERS             = 91,
    SI5351_REGISTER_92_CLOCK_6_7_OUTPUT_DIVIDER           = 92,
    SI5351_REGISTER_165_CLK0_INITIAL_PHASE_OFFSET         = 165,
    SI5351_REGISTER_166_CLK1_INITIAL_PHASE_OFFSET         = 166,
    SI5351_REGISTER_167_CLK2_INITIAL_PHASE_OFFSET         = 167,
    SI5351_REGISTER_168_CLK3_INITIAL_PHASE_OFFSET         = 168,
    SI5351_REGISTER_169_CLK4_INITIAL_PHASE_OFFSET         = 169,
    SI5351_REGISTER_170_CLK5_INITIAL_PHASE_OFFSET         = 170,
    SI5351_REGISTER_177_PLL_RESET                         = 177,
    SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE = 183
};

typedef enum {
    SI5351_CRYSTAL_LOAD_6PF  = (1<<6),
    SI5351_CRYSTAL_LOAD_8PF  = (2<<6),
    SI5351_CRYSTAL_LOAD_10PF = (3<<6)
} si5351CrystalLoad_t;

typedef enum {
    SI5351_PLL_A = 0,
    SI5351_PLL_B,
} si5351PLL_t;

typedef enum {
    SI5351_R_DIV_1   = 0,
    SI5351_R_DIV_2   = 1,
    SI5351_R_DIV_4   = 2,
    SI5351_R_DIV_8   = 3,
    SI5351_R_DIV_16  = 4,
    SI5351_R_DIV_32  = 5,
    SI5351_R_DIV_64  = 6,
    SI5351_R_DIV_128 = 7,
} si5351RDiv_t;

typedef enum {
    SI5351_DRIVE_STRENGTH_2MA = 0x00, //  ~ 2.2 dBm
    SI5351_DRIVE_STRENGTH_4MA = 0x01, //  ~ 7.5 dBm
    SI5351_DRIVE_STRENGTH_6MA = 0x02, //  ~ 9.5 dBm
    SI5351_DRIVE_STRENGTH_8MA = 0x03, // ~ 10.7 dBm
} si5351DriveStrength_t;

typedef struct {
    u32 mult;
    u32 num;
    u32 denom;
} si5351PLLConfig_t;

typedef struct {
    u8 allowIntegerMode;
    u32 div;
    u32 num;
    u32 denom;
    si5351RDiv_t rdiv;
} si5351OutputConfig_t;

u32 si5351Correction;

// Xilinx I2C Write for common use
u32 si5351_write(XIicPs* pIic,u8 reg_addr, u8 reg_value)
{
    u8 data[2] = {reg_addr, reg_value};
    u32 Status = XIicPs_MasterSendPolled(pIic, data, sizeof(data), ADDR_SI5351);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    else
        return XST_SUCCESS;
}

// Common code for _SetupPLL and _SetupOutput
u32 si5351_writeBulk(XIicPs* pIic,u8 baseaddr, u32 P1, u32 P2, u32 P3, u8 divBy4, si5351RDiv_t rdiv) {
    u32 Status;
    Status = si5351_write(pIic, baseaddr,   (P3 >> 8) & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+1, P3 & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+2, ((P1 >> 16) & 0x3) | ((divBy4 & 0x3) << 2) | ((rdiv & 0x7) << 4));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+3, (P1 >> 8) & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+4, P1 & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+5, ((P3 >> 12) & 0xF0) | ((P2 >> 16) & 0xF));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+6, (P2 >> 8) & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic, baseaddr+7, P2 & 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    return XST_SUCCESS;
}

/*
 * Initializes Si5351. Call this function before doing anything else.
 * `Correction` is the difference of actual frequency and desired frequency @ 100 MHz.
 * It can be measured at lower frequencies and scaled linearly.
 * E.g. if you get 10_000_097 Hz instead of 10_000_000 Hz, `correction` is 97*10 = 970
 */
u32 si5351_Init(XIicPs* pIic,u32 correction) {
    si5351Correction = correction;
    u32 Status;
    // Disable all outputs by setting CLKx_DIS high
    Status = si5351_write(pIic,SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, 0xFF);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    // Power down all output drivers
    Status = si5351_write(pIic,SI5351_REGISTER_16_CLK0_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_17_CLK1_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_18_CLK2_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_19_CLK3_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_20_CLK4_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_21_CLK5_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_22_CLK6_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,SI5351_REGISTER_23_CLK7_CONTROL, 0x80);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    // Set the load capacitance for the XTAL
    si5351CrystalLoad_t crystalLoad = SI5351_CRYSTAL_LOAD_10PF;
    Status = si5351_write(pIic,SI5351_REGISTER_183_CRYSTAL_INTERNAL_LOAD_CAPACITANCE, crystalLoad);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    return XST_SUCCESS;
}

// Sets the multiplier for given PLL
u32 si5351_SetupPLL(XIicPs* pIic,si5351PLL_t pll, si5351PLLConfig_t* conf) {
    u32 P1, P2, P3;
    u32 Status;
    u32 mult = conf->mult;
    u32 num = conf->num;
    u32 denom = conf->denom;

    P1 = 128 * mult + (128 * num)/denom - 512;
    // P2 = 128 * num - denom * ((128 * num)/denom);
    P2 = (128 * num) % denom;
    P3 = denom;

    // Get the appropriate base address for the PLL registers
    u8 baseaddr = (pll == SI5351_PLL_A ? 26 : 34);
    Status = si5351_writeBulk(pIic,baseaddr, P1, P2, P3, 0, 0);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    // Reset both PLLs
    Status = si5351_write(pIic,SI5351_REGISTER_177_PLL_RESET, (1<<7) | (1<<5) );
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    return XST_SUCCESS;
}

// Configures PLL source, drive strength, multisynth divider, Rdivider and phaseOffset.
// Returns 0 on success, != 0 otherwise.
u32 si5351_SetupOutput(XIicPs* pIic,u8 output, si5351PLL_t pllSource, si5351DriveStrength_t driveStrength, si5351OutputConfig_t* conf, u8 phaseOffset) {
    u32 div = conf->div;
    u32 num = conf->num;
    u32 denom = conf->denom;
    u8 divBy4 = 0;
    u32 P1, P2, P3;
    u32 Status;

    if(output > 2) {
        return 1;
    }

    if((!conf->allowIntegerMode) && ((div < 8) || ((div == 8) && (num == 0)))) {
        // div in { 4, 6, 8 } is possible only in integer mode
        return 2;
    }

    if(div == 4) {
        // special DIVBY4 case, see AN619 4.1.3
        P1 = 0;
        P2 = 0;
        P3 = 1;
        divBy4 = 0x3;
    } else {
        P1 = 128 * div + ((128 * num)/denom) - 512;
        // P2 = 128 * num - denom * (128 * num)/denom;
        P2 = (128 * num) % denom;
        P3 = denom;
    }

    // Get the register addresses for given channel
    u8 baseaddr = 0;
    u8 phaseOffsetRegister = 0;
    u8 clkControlRegister = 0;
    switch (output) {
    case 0:
        baseaddr = SI5351_REGISTER_42_MULTISYNTH0_PARAMETERS_1;
        phaseOffsetRegister = SI5351_REGISTER_165_CLK0_INITIAL_PHASE_OFFSET;
        clkControlRegister = SI5351_REGISTER_16_CLK0_CONTROL;
        break;
    case 1:
        baseaddr = SI5351_REGISTER_50_MULTISYNTH1_PARAMETERS_1;
        phaseOffsetRegister = SI5351_REGISTER_166_CLK1_INITIAL_PHASE_OFFSET;
        clkControlRegister = SI5351_REGISTER_17_CLK1_CONTROL;
        break;
    case 2:
        baseaddr = SI5351_REGISTER_58_MULTISYNTH2_PARAMETERS_1;
        phaseOffsetRegister = SI5351_REGISTER_167_CLK2_INITIAL_PHASE_OFFSET;
        clkControlRegister = SI5351_REGISTER_18_CLK2_CONTROL;
        break;
    }

    u8 clkControl = 0x0C | driveStrength; // clock not inverted, powered up
    if(pllSource == SI5351_PLL_B) {
        clkControl |= (1 << 5); // Uses PLLB
    }

    if((conf->allowIntegerMode) && ((num == 0)||(div == 4))) {
        // use integer mode
        clkControl |= (1 << 6);
    }

    Status = si5351_write(pIic,clkControlRegister, clkControl);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_writeBulk(pIic,baseaddr, P1, P2, P3, divBy4, conf->rdiv);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    Status = si5351_write(pIic,phaseOffsetRegister, (phaseOffset & 0x7F));
    if (Status != XST_SUCCESS)
        return XST_FAILURE;

    return XST_SUCCESS;
}

// Calculates PLL, MS and RDiv settings for given Fclk in [8_000, 160_000_000] range.
// The actual frequency will differ less than 6 Hz from given Fclk, assuming `correction` is right.
void si5351_Calc(u32 Fclk, si5351PLLConfig_t* pll_conf, si5351OutputConfig_t* out_conf) {
    if(Fclk < 8000) Fclk = 8000;
    else if(Fclk > 160000000) Fclk = 160000000;

    out_conf->allowIntegerMode = 1;

    if(Fclk < 1000000) {
        // For frequencies in [8_000, 500_000] range we can use si5351_Calc(Fclk*64, ...) and SI5351_R_DIV_64.
        // In practice it's worth doing for any frequency below 1 MHz, since it reduces the error.
        Fclk *= 64;
        out_conf->rdiv = SI5351_R_DIV_64;
    } else {
        out_conf->rdiv = SI5351_R_DIV_1;
    }

    // Apply correction, _after_ determining rdiv.
    Fclk = Fclk - (u32)((((double)Fclk)/100000000.0)*((double)si5351Correction));

    // Here we are looking for integer values of a,b,c,x,y,z such as:
    // N = a + b / c    # pll settings
    // M = x + y / z    # ms  settings
    // Fclk = Fxtal * N / M
    // N in [24, 36]
    // M in [8, 1800] or M in {4,6}
    // b < c, y < z
    // b,c,y,z <= 2**20
    // c, z != 0
    // For any Fclk in [500K, 160MHz] this algorithm finds a solution
    // such as abs(Ffound - Fclk) <= 6 Hz

    const u32 Fxtal = XREF_FREQ;
    u32 a, b, c, x, y, z, t;

    if(Fclk < 81000000) {
        // Valid for Fclk in 0.5..112.5 MHz range
        // However an error is > 6 Hz above 81 MHz
        a = 36; // PLL runs @ 900 MHz
        b = 0;
        c = 1;
        u32 Fpll = 900000000;
        x = Fpll/Fclk;
        t = (Fclk >> 20) + 1;
        y = (Fpll % Fclk) / t;
        z = Fclk / t;
    } else {
        // Valid for Fclk in 75..160 MHz range
        if(Fclk >= 150000000) {
            x = 4;
        } else if (Fclk >= 100000000) {
            x = 6;
        } else {
            x = 8;
        }
        y = 0;
        z = 1;

        u32 numerator = x*Fclk;
        a = numerator/Fxtal;
        t = (Fxtal >> 20) + 1;
        b = (numerator % Fxtal) / t;
        c = Fxtal / t;
    }

    pll_conf->mult = a;
    pll_conf->num = b;
    pll_conf->denom = c;
    out_conf->div = x;
    out_conf->num = y;
    out_conf->denom = z;
}


// Setup CLK0 for given frequency and drive strength. Use PLLA.
u32 si5351_SetupCLK0(XIicPs* pIic, u32 Fclk, si5351DriveStrength_t driveStrength) {
    u32 Status;
	si5351PLLConfig_t pll_conf;
	si5351OutputConfig_t out_conf;

	si5351_Calc(Fclk, &pll_conf, &out_conf);
	Status = si5351_SetupPLL(pIic,SI5351_PLL_A, &pll_conf);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
	Status = si5351_SetupOutput(pIic,0, SI5351_PLL_A, driveStrength, &out_conf, 0);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    return XST_SUCCESS;
}

// Setup CLK2 for given frequency and drive strength. Use PLLB.
u32 si5351_SetupCLK2(XIicPs* pIic,u32 Fclk, si5351DriveStrength_t driveStrength) {
	u32 Status;
    si5351PLLConfig_t pll_conf;
	si5351OutputConfig_t out_conf;

	si5351_Calc(Fclk, &pll_conf, &out_conf);
	Status = si5351_SetupPLL(pIic,SI5351_PLL_B, &pll_conf);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
	Status = si5351_SetupOutput(pIic,2, SI5351_PLL_B, driveStrength, &out_conf, 0);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    return XST_SUCCESS;
}

// Enables or disables outputs depending on provided bitmask.
// Examples:
// si5351_EnableOutputs(1 << 0) enables CLK0 and disables CLK1 and CLK2
// si5351_EnableOutputs((1 << 2) | (1 << 0)) enables CLK0 and CLK2 and disables CLK1
u32 si5351_EnableOutputs(XIicPs* pIic,u8 enabled) {
    u32 Status;
    Status = si5351_write(pIic,SI5351_REGISTER_3_OUTPUT_ENABLE_CONTROL, ~enabled);
    if (Status != XST_SUCCESS)
        return XST_FAILURE;
    return XST_SUCCESS;
}
u32 SetMacAddress()
{
    XIicPs Iic;
    XIicPs_Config *IicConfig;
    XEmacPs Emac;
    XEmacPs_Config *EmacConfig;
    u32 Status, i;
    u8 Buffer[1024];
    u32 freq_set;
    u32 freq_out_set;
    char *Pointer;

    Buffer[0] = 0x18;
    Buffer[1] = 0;
    xil_printf("User RedPitaya Bootloader start\r\n");

    IicConfig = XIicPs_LookupConfig(XPAR_PS7_I2C_0_DEVICE_ID);
    if(IicConfig == NULL) return XST_FAILURE;
    Status = XIicPs_CfgInitialize(&Iic, IicConfig, IicConfig->BaseAddress);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    Status = XIicPs_SetSClk(&Iic, 100000);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    Status = XIicPs_MasterSendPolled(&Iic, Buffer, 2, ADDR_EEPROM);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    while(XIicPs_BusIsBusy(&Iic));
    Status = XIicPs_MasterRecvPolled(&Iic, Buffer, 1024, ADDR_EEPROM);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    Pointer = memmem(Buffer, 1024, "ethaddr=", 8);
    if(Pointer == NULL) return XST_FAILURE;
    Pointer += 7;
    xil_printf("MAC address read from EEPROM:\r\n");
    for(i = 0; i < 6; ++i)
    {
        Buffer[i] = strtol(Pointer + 1, &Pointer, 16);
        xil_printf("%2x",Buffer[i]);
        if(i < 5)
            xil_printf(":");
    }

    xil_printf("\r\n");

    EmacConfig = XEmacPs_LookupConfig(XPAR_PS7_ETHERNET_0_DEVICE_ID);
    if(EmacConfig == NULL) return XST_FAILURE;
    Status = XEmacPs_CfgInitialize(&Emac, EmacConfig, EmacConfig->BaseAddress);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    Status = XEmacPs_SetMacAddress(&Emac, Buffer, 1);
    if(Status != XST_SUCCESS) return XST_FAILURE;
    xil_printf("Bootloader Si5351 config\r\n");
    Pointer = memmem(Buffer, 1024, "clk_freq=", 9);
    if(Pointer != NULL) {
        Pointer += 8;
        freq_set = strtol(Pointer + 1, &Pointer, 10);
        xil_printf("EEPROM adc clk freq set :%dHz\r\n",freq_set);
    }
    else {
    	xil_printf("EEPROM read adc clk freq failed.Set to default 125Mhz\r\n");
        freq_set = 125000000;
    }

    Pointer = memmem(Buffer, 1024, "clk_out_freq=", 13);
    if(Pointer != NULL) {
        Pointer += 12;
        freq_out_set = strtol(Pointer + 1, &Pointer, 10);
        xil_printf("EEPROM clk out freq set :%dHz\r\n",freq_out_set);
    }
    else {
    	xil_printf("EEPROM read clk out freq failed.Set to default 10Mhz\r\n");
        freq_out_set = 10000000;
    }

    /* Set Si5351 to Set freq */
    Status = si5351_Init(&Iic,correction);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    Status = si5351_SetupCLK0(&Iic,freq_set, SI5351_DRIVE_STRENGTH_6MA);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    Status = si5351_SetupCLK2(&Iic,freq_out_set, SI5351_DRIVE_STRENGTH_6MA);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    Status = si5351_EnableOutputs(&Iic,(1<<0) | (1<<2));
    if (Status != XST_SUCCESS) return XST_FAILURE;
    xil_printf("Bootloader config success,boot to Linux\r\n");
    return XST_SUCCESS;
}


