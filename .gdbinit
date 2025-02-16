#==============================================================================
# (c) Copyright 2018 Microsemi Corporation. All rights reserved.
#
# softconsole.gdbinit
#
# SoftConsole default debug initialization commands.
#
# SVN $Revision: 6662 $
# SVN $Date: 2014-07-03 15:05:36 +0100 (Thu, 03 Jul 2014) $
#==============================================================================


#------------------------------------------------------------------------------
# check-target-poke-arm
#
# Detecing what ARM and what Cortex-M? type is the target. Doing this on some
# riscv targets might cause the target to misbehave, so be careful using this
# with riscv.
#------------------------------------------------------------------------------
define check-target-poke-arm

    # Read CPUID Base Register and extract field values
    set $cpuid              = *(0xe000ed00)
    set $cpuid_implementer  = (($cpuid >> 24) & 0x000000ff)
    set $cpuid_variant      = (($cpuid >> 20) & 0x0000000f)
    set $cpuid_architecture = (($cpuid >> 16) & 0x0000000f)
    set $cpuid_partno       = (($cpuid >> 4)  & 0x00000fff)
    set $cpuid_revision     = ( $cpuid        & 0x0000000f)

    # Cortex-M? Which type?
    set $cortex_m = ($cpuid_implementer == 0x41)
    if ($cortex_m)
        set $cortex_m0plus  = ($cpuid_partno == 0xc60)
        set $cortex_m0      = ($cpuid_partno == 0xc20)
        set $cortex_m1      = ($cpuid_partno == 0xc21)
        set $cortex_m3      = ($cpuid_partno == 0xc23)
        set $cortex_m4      = ($cpuid_partno == 0xc24)
        set $cortex_m7      = ($cpuid_partno == 0xc27)
        set $cortex_m23     = ($cpuid_partno == 0xd20)
        # TODO: what is the Cortex-M33 partno?
        set $cortex_m33     = ($cpuid_partno == 0xfff)

        # SmartFusion2? i.e. Cortex-M3 r2p1
        set $smartfusion2 = ($cortex_m3 && ($cpuid_variant == 2) && ($cpuid_revision == 1))
    else
        # If Cortex-M not detected then assume RISC-V
        # TODO: need to revisit/review this
        set $riscv = 1
    end
end


#------------------------------------------------------------------------------
# check-target
#
# Contains 2 different implementations, the target specific will require to be 
# told the generic clasification of a target (riscv/arm). Which requires the
# projects to be updated and feeding this information to gdb with:
# gdb -ex 'set $target_riscv = 1'
# This needs to be reflected in the SoftConsole projects, but should work without
# issues on all targets. 
#
# While older automatic-detector implementation doesn't require anything but
# can cause problems with some riscv targets.
#
# Check CPU/FPGA type using Cortex-M System Control Block (SCB) CPUID register.
# 
# CPUID register TRM documentation
# --------------------------------
# Cortex-M0+: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0484c/Bhccjgga.html
# Cortex-M0:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0432c/Bhccjgga.html
# Cortex-M1:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0413d/Cihhbddh.html
# Cortex-M3:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100165_0201_00_en/ric1417175933150.html
# Cortex-M4:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100166_0001_00_en/ric1417175933150.html
# Cortex-M7:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0489d/Cihhbddh.html
#
# CPUID register @ 0xe000ed00 fields
# ----------------------------------
# cpuid[31:24] = implementer[7:0]  (shift 24, mask 0x000000ff)
# cpuid[23:20] = variant[3:0]      (shift 20, mask 0x0000000f)
# cpuid[19:16] = architecture[3:0] (shift 16, mask 0x0000000f)
# cpuid[15:4]  = partno[11:0]      (shift 4,  mask 0x00000fff)
# cpuid[3:0]   = revision[3:0]     (shift 0,  mask 0x0000000f)
#
# CPUID field values
# ------------------
# implementer[7:0]  = 0x41 for ARM
# variant[3:0]      = the N in rNpM
# architecture[3:0] = 0xc (ARMv6-M = Cortex-M0+/0/1), 0x0f (ARMv7-M = Cortex-M3/4/7)
# partno[11:0]      = 0xc60 (M0+), 0xc20 (M0), 0xc21 (M1), 0xc23* (M3), 
#                     0xc24 (M4), 0xc27 (M7), 0xd20 (M23), ??? (M33)
# revision[3:0]     = the M in rNpM
# * Note: Cortex-M3 r2p1 TRM incorrectly says 0xc24. r2p0 TRM correctly says 0xc23 
#  (http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0337h/Cihhbddh.html)
#   as does Generic User Guide 
#  (http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/CIHCAGHH.html)
# 
# FPGA type
# ---------
# SmartFusion is Cortex-M3 r1p1* (variant = 1, revision = 1)
# SmartFusion2 is Cortex-M3 r2p1 (variant = 2, revision = 1)
# * Note: or earlier for some early devices?
#
# Variables set on exit by this function
# --------------------------------------
# $cortex_m      = 1 if Cortex-M detected,     0 if not
# $cortex_m0plus = 1 if Cortex-M0+ detected,   0 if not
# $cortex_m0     = 1 if Cortex-M0 detected,    0 if not
# $cortex_m1     = 1 if Cortex-M1 detected,    0 if not
# $cortex_m3     = 1 if Cortex-M3 detected,    0 if not
# $cortex_m4     = 1 if Cortex-M4 detected,    0 if not
# $cortex_m7     = 1 if Cortex-M7 detected,    0 if not
# $cortex_m23    = 1 if Cortex-M23 detected,   0 if not
# $cortex_m33    = 1 if Cortex-M33 detected,   0 if not
# $smartfusion2  = 1 if SmartFusion2 detected, 0 if not
# $riscv         = 1 if RISC-V detected,       0 if not
#------------------------------------------------------------------------------
define check-target

    if ($target_riscv)
        # Trust it on the face value - do nothing special for riscv, do not poke plic, nor arm-cpuid.
        set $riscv = 1
    else 
        # No else-if so this is ugly/nested
        if ($target_arm)
            # Trust it's an arm, but check what type/cortex-m is it exactly.
            # The inner function will set $riscv or $cortex_m acordingly (plus some other variables)
            check-target-poke-arm
        else
            # When target is not specified then fallback to original previous
            # behaviour in such way that older projects will behave the way they did in the past 

            # First check for RISC-V by checking for "plic {\n " at start of config 
            # string starting at 0x00001020.
            # TODO: generalise this for an arbitrary RISC-V target as this is specific
            # to the initial Microsemi RV32IM (SiFive E31) CPU which uses an early/non-
            # standard approach to storing the config string
            set $riscv = (((*0x00001020) == 0x63696c70) && ((*0x00001024) == 0x200a7b20))

            # If RISC-V not detected then proceed to Cortex-M checking    
            if (!$riscv) 
                check-target-poke-arm
            end
        end
    end

end


#------------------------------------------------------------------------------
# hookpost-symbol-file
#
# Target initialization required before program has been downloaded to target. 
# This hook executes automatically when the GDB symbol-file command is invoked 
# just after it has done its own work. See here for more on GDB user defined 
# command hooks:
# http://sourceware.org/gdb/current/onlinedocs/gdb/Hooks.html#Hooks 
#------------------------------------------------------------------------------

define hookpost-symbol-file

    check-target

    # Notes regarding envm memory mapping to CODE space for SmartFusion and 
    # SmartFusion2 below.
    #
    # Tell gdb that envm @ 0x00000000 is read only so that it uses hardware
    # breakpoints. By default the memory regions fed to gdb by openocd
    # will be:
    # 
    # 0  0x00000000    0x5fffffff      rw
    # 1  0x60000000    <top of envm>   ro (flash)
    # 2  <top of envm> <top of memory> rw
    # 
    # So here we delete mem region #0 and replace it with the mem regions
    # below. 
    # 
    # Notes/caveats:
    # (1) For simplicity the envm ro memory region size is that of the 
    # largest devices - i.e. SmartFusion A2F500 and SmartFusion2 M2S500 = 
    # 512KiB. Smaller devices may have less envm. 
    # (2) SmartFusion and SmartFusion2 cater for more flexible memory 
    # remapping scenarios than are catered for by this script. E.g. envm 
    # partitioning and remapping to addresses other than 0x00000000. 
    # (3) This script assumes that the three memory regions described above 
    # are defined a priori and are numbered 0-2.

    if ($smartfusion)
        # See comments above
        delete mem 0
        mem 0x00000000 0x0007ffff ro
        mem 0x00080000 0x5fffffff rw
    end

    if ($smartfusion2)

        # SmartFusion2 remapping
        # Check value of __smartfusion2_memory_remap defined in linker script
        set $SF2_REMAP_ENVM  = 0
        set $SF2_REMAP_ESRAM = 1
        set $SF2_REMAP_DDR   = 2

        if (&__smartfusion2_memory_remap == $SF2_REMAP_ENVM)
            # See comments above
            delete mem 0
            mem 0x00000000 0x0007ffff ro
            mem 0x00080000 0x5fffffff rw
        end

        if (&__smartfusion2_memory_remap == $SF2_REMAP_DDR)        
            # SmartFusion2 register addresses & values
            set $SF2_ESRAM_CR       = 0x40038000
            set $SF2_DDR_CR         = 0x40038008
            set $SF2_ENVM_REMAP_CR  = 0x40038010
            set $SF2_FLUSH_CR       = 0x400381a8

            # Remap external DDR memory to address 0x00000000
            # and flush the cache in case it is enabled
            set *$SF2_ESRAM_CR      = 0x00000000
            set *$SF2_ENVM_REMAP_CR = 0x00000000
            set *$SF2_DDR_CR        = 0x00000001
            set *$SF2_FLUSH_CR      = 0x00000001
        end

        # Nothing to do for &__smartfusion2_memory_remap == $SF2_REMAP_ESRAM
    end

    if ($riscv)
        # Basic check for Renode first (initial $pc == 0x00001000) to work
        # around problems with reading $misa from Renode. When the latter is
        # fixed this special check for Renode can be removed and the regular
        # check for a RV64 target can be done regardless of whether the target
        # type is real hardware or Renode emulation.
        if ($pc != 0x00001000)
            # Read $misa (Machine ISA CSR) to check if XLEN == 64
            # ($misa[63:62] == 2) and, if so, assume target is PolarFire
            # SoC in which case define eNVM memory region (128KiB @ 0x20220000
            # - 0x2023ffff) as read-only so that debugging automatically uses 
            # hardware breakpoints.
            set $mpfs = ((($misa >> 62) & 0x3) == 2)
            if ($mpfs)
                mem 0x20220000 0x2023ffff ro
            end
        end
    end

end

#------------------------------------------------------------------------------
# hookpost-load
# 
# Target initialization required after program has been downloaded to target. 
# This hook executes automatically when the GDB load command is invoked just 
# after it has done its own work. See here for more on GDB user defined 
# command hooks:
# http://sourceware.org/gdb/current/onlinedocs/gdb/Hooks.html#Hooks 
#------------------------------------------------------------------------------

define hookpost-load

    check-target

    if ($cortex_m)

        # gdb load automatically initializes the PC register from program's 
        # vector table but we need to manually initialize SP register from the 
        # program's vector table. For targets that support the System Control
        # Block (SCB) Vector Table Offset Register (VTOR) @ 0xe000ed08 we need 
        # to initialize the VTOR with the base address of the vector table given 
        # by the linker script symbol __vector_table_vma_base_address. This 
        # ensures that that ISRs/exception handlers work properly during 
        # debugging. For targets that do no support the VTOR or a relocateable 
        # vector table the vector table base address is assumed to be 0x00000000. 

        # Cortex-M0+/3/4/7 support VTOR so program it and set initial SP
        # Cortex-M0+: http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0484c/Cihjajhi.html
        # Cortex-M3:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100165_0201_00_en/ric1414056339702.html
        # Cortex-M4:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.100166_0001_00_en/ric1417011873697.html
        # Cortex-M7:  http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0489d/BABCIIIA.html
        #
        # Note: we don't initialize the VTOR for SmartFusion (Cortex-M3) because
        # the system boot code does this already.      
        if ($cortex_m0plus || ($cortex_m3 && $smartfusion2) || $cortex_m4 || $cortex_m7)
            set $sp = *((unsigned long *)&__vector_table_vma_base_address)
            set *0xe000ed08 = &__vector_table_vma_base_address
        end

        # Cortex-M0/1 don't support VTOR so vector table assumed to be @ 0x00000000
        if ($cortex_m1 || $cortex_m0)
            set $sp = *((unsigned long *)0x00000000)
        end

        # Tell gdb how many hardware breakpoints & watchpoints are available
        # Read this info from the Cortex-M Flash Patch (@ 0xe0002000) and DWT 
        # (@ 0xe0001000) Control Registers
        set $num_bps = (((*0xe0002000) >> 4)  & 0x0000000f)
        set $num_wps = (((*0xe0001000) >> 28) & 0x0000000f)
        set remote hardware-breakpoint-limit $num_bps
        set remote hardware-watchpoint-limit $num_wps
    end

    # TODO: tell gdb how many RISC-V hardware breakpoints/watchpoints available
end


#------------------------------------------------------------------------------
# plot_variable
#
# Will dump content of $arg0 variable into a file, then invoke scplot to process it
# https://github.com/gonum/plot
#------------------------------------------------------------------------------

define plot_variable
  # Making sure the directory is created and no errors shown when it already exists
  shell touch temp
  shell rm -rf temp
  shell mkdir temp
  set logging file temp/plot_varaible.raw
  set logging on
  output $arg0
  set logging off
  shell $SC_INSTALL_DIR/extras/scplot/scplot -input temp/plot_varaible.raw
end

# target remote localhost:3333
# # Reset the target
# monitor reset halt
# # Set breakpoint at main
# break main
