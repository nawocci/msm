load(":target_variants.bzl", "la_variants")
load(":msm_kernel_la.bzl", "define_msm_la")
load(":image_opts.bzl", "boot_image_opts")

target_name = "pineapple"
target_arch = "pineapple"

def define_pineapple():
    _pineapple_in_tree_modules = [
        # keep sorted
        "arch/arm64/gunyah/gh_arm_drv.ko",
        "drivers/bus/mhi/devices/mhi_dev_satellite.ko",
        "drivers/bus/mhi/devices/mhi_dev_uci.ko",
        "drivers/bus/mhi/host/mhi.ko",
        "drivers/clk/qcom/camcc-pineapple.ko",
        "drivers/clk/qcom/clk-dummy.ko",
        "drivers/clk/qcom/clk-qcom.ko",
        "drivers/clk/qcom/clk-rpmh.ko",
        "drivers/clk/qcom/dispcc-pineapple.ko",
        "drivers/clk/qcom/gcc-pineapple.ko",
        "drivers/clk/qcom/gdsc-regulator.ko",
        "drivers/clk/qcom/gpucc-pineapple.ko",
        "drivers/clk/qcom/tcsrcc-pineapple.ko",
        "drivers/clk/qcom/videocc-pineapple.ko",
        "drivers/cpufreq/qcom-cpufreq-hw.ko",
        "drivers/devfreq/governor_gpubw_mon.ko",
        "drivers/devfreq/governor_msm_adreno_tz.ko",
        "drivers/dma-buf/heaps/qcom_dma_heaps.ko",
        "drivers/dma/qcom/bam_dma.ko",
        "drivers/dma/qcom/msm_gpi.ko",
        "drivers/firmware/arm_scmi/qcom_scmi_vendor.ko",
        "drivers/firmware/qcom-scm.ko",
        "drivers/gpu/drm/display/drm_display_helper.ko",
        "drivers/gpu/drm/display/drm_dp_aux_bus.ko",
        "drivers/hwspinlock/qcom_hwspinlock.ko",
        "drivers/i2c/busses/i2c-msm-geni.ko",
        "drivers/i3c/master/i3c-master-msm-geni.ko",
        "drivers/iio/adc/qti-glink-adc.ko",
        "drivers/input/misc/pm8941-pwrkey.ko",
        "drivers/interconnect/qcom/icc-bcm-voter.ko",
        "drivers/interconnect/qcom/icc-debug.ko",
        "drivers/interconnect/qcom/icc-rpmh.ko",
        "drivers/interconnect/qcom/qnoc-pineapple.ko",
        "drivers/interconnect/qcom/qnoc-qos.ko",
        "drivers/iommu/arm/arm-smmu/arm_smmu.ko",
        "drivers/iommu/iommu-logger.ko",
        "drivers/iommu/msm_dma_iommu_mapping.ko",
        "drivers/iommu/qcom_iommu_debug.ko",
        "drivers/iommu/qcom_iommu_util.ko",
        "drivers/irqchip/qcom-pdc.ko",
        "drivers/leds/leds-qti-flash.ko",
        "drivers/leds/rgb/leds-qcom-lpg.ko",
        "drivers/mailbox/msm_qmp.ko",
        "drivers/mailbox/qcom-ipcc.ko",
        "drivers/mfd/qcom-spmi-pmic.ko",
        "drivers/misc/qseecom_proxy.ko",
        "drivers/nvmem/nvmem_qcom-spmi-sdam.ko",
        "drivers/nvmem/nvmem_qfprom.ko",
        "drivers/pci/controller/pci-msm-drv.ko",
        "drivers/perf/qcom_llcc_pmu.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs.ko",
        "drivers/phy/qualcomm/phy-qcom-ufs-qmp-v4-pineapple.ko",
        "drivers/pinctrl/qcom/pinctrl-msm.ko",
        "drivers/pinctrl/qcom/pinctrl-pineapple.ko",
        "drivers/pinctrl/qcom/pinctrl-spmi-gpio.ko",
        "drivers/pinctrl/qcom/pinctrl-spmi-mpp.ko",
        "drivers/power/reset/qcom-dload-mode.ko",
        "drivers/power/reset/qcom-pon.ko",
        "drivers/power/reset/qcom-reboot-reason.ko",
        "drivers/power/reset/reboot-mode.ko",
        "drivers/power/supply/qti_battery_charger.ko",
        "drivers/regulator/debug-regulator.ko",
        "drivers/regulator/proxy-consumer.ko",
        "drivers/regulator/qti-fixed-regulator.ko",
        "drivers/regulator/rpmh-regulator.ko",
        "drivers/remoteproc/qcom_pil_info.ko",
        "drivers/remoteproc/qcom_q6v5.ko",
        "drivers/remoteproc/qcom_q6v5_pas.ko",
        "drivers/remoteproc/qcom_spss.ko",
        "drivers/remoteproc/qcom_sysmon.ko",
        "drivers/remoteproc/rproc_qcom_common.ko",
        "drivers/rpmsg/glink_pkt.ko",
        "drivers/rpmsg/qcom_glink.ko",
        "drivers/rpmsg/qcom_glink_rpm.ko",
        "drivers/rpmsg/qcom_glink_smem.ko",
        "drivers/rpmsg/qcom_glink_spss.ko",
        "drivers/rpmsg/qcom_smd.ko",
        "drivers/rtc/rtc-pm8xxx.ko",
        "drivers/slimbus/slim-qcom-ngd-ctrl.ko",
        "drivers/slimbus/slimbus.ko",
        "drivers/soc/qcom/altmode-glink.ko",
        "drivers/soc/qcom/charger-ulog-glink.ko",
        "drivers/soc/qcom/cmd-db.ko",
        "drivers/soc/qcom/cpucp_log.ko",
        "drivers/soc/qcom/crm.ko",
        "drivers/soc/qcom/dcc_v2.ko",
        "drivers/soc/qcom/dcvs/bwmon.ko",
        "drivers/soc/qcom/dcvs/c1dcvs_scmi_v2.ko",
        "drivers/soc/qcom/dcvs/cpufreq_stats_scmi_v2.ko",
        "drivers/soc/qcom/dcvs/dcvs_fp.ko",
        "drivers/soc/qcom/dcvs/memlat.ko",
        "drivers/soc/qcom/dcvs/qcom-dcvs.ko",
        "drivers/soc/qcom/dcvs/qcom-pmu-lib.ko",
        "drivers/soc/qcom/dcvs/qcom_scmi_client.ko",
        "drivers/soc/qcom/debug_symbol.ko",
        "drivers/soc/qcom/llcc-qcom.ko",
        "drivers/soc/qcom/mdt_loader.ko",
        "drivers/soc/qcom/mem_buf/mem_buf.ko",
        "drivers/soc/qcom/mem_buf/mem_buf_dev.ko",
        "drivers/soc/qcom/mem_buf/mem_buf_msgq.ko",
        "drivers/soc/qcom/memory_dump_v2.ko",
        "drivers/soc/qcom/minidump.ko",
        "drivers/soc/qcom/msm_performance.ko",
        "drivers/soc/qcom/pdr_interface.ko",
        "drivers/soc/qcom/pmic-glink-debug.ko",
        "drivers/soc/qcom/qcom_aoss.ko",
        "drivers/soc/qcom/qcom_cpucp.ko",
        "drivers/soc/qcom/qcom_ice.ko",
        "drivers/soc/qcom/qcom_logbuf_boot_log.ko",
        "drivers/soc/qcom/qcom_logbuf_vendor_hooks.ko",
        "drivers/soc/qcom/qcom_ramdump.ko",
        "drivers/soc/qcom/qcom_rpmh.ko",
        "drivers/soc/qcom/qcom_va_minidump.ko",
        "drivers/soc/qcom/qcom_wdt_core.ko",
        "drivers/soc/qcom/qmi_helpers.ko",
        "drivers/soc/qcom/qti_battery_debug.ko",
        "drivers/soc/qcom/qti_pmic_glink.ko",
        "drivers/soc/qcom/secure_buffer.ko",
        "drivers/soc/qcom/smem.ko",
        "drivers/soc/qcom/smp2p.ko",
        "drivers/soc/qcom/socinfo.ko",
        "drivers/soc/qcom/sps/sps_drv.ko",
        "drivers/soc/qcom/wcd_usbss_i2c.ko",
        "drivers/spi/q2spi-geni.ko",
        "drivers/spi/spi-msm-geni.ko",
        "drivers/spmi/spmi-pmic-arb.ko",
        "drivers/thermal/qcom/qcom-spmi-temp-alarm.ko",
        "drivers/tty/hvc/hvc_gunyah.ko",
        "drivers/tty/serial/msm_geni_serial.ko",
        "drivers/ufs/host/ufs-qcom.ko",
        "drivers/ufs/host/ufshcd-crypto-qti.ko",
        "drivers/uio/msm_sharedmem/msm_sharedmem.ko",
        "drivers/usb/dwc3/dwc3-msm.ko",
        "drivers/usb/gadget/function/usb_f_ccid.ko",
        "drivers/usb/gadget/function/usb_f_cdev.ko",
        "drivers/usb/gadget/function/usb_f_gsi.ko",
        "drivers/usb/gadget/function/usb_f_qdss.ko",
        "drivers/usb/phy/phy-msm-snps-eusb2.ko",
        "drivers/usb/phy/phy-msm-ssusb-qmp.ko",
        "drivers/usb/phy/phy-qcom-emu.ko",
        "drivers/usb/repeater/repeater.ko",
        "drivers/usb/repeater/repeater-qti-pmic-eusb2.ko",
        "drivers/usb/typec/ucsi/ucsi_qti_glink.ko",
        "drivers/virt/gunyah/gh_ctrl.ko",
        "drivers/virt/gunyah/gh_dbl.ko",
        "drivers/virt/gunyah/gh_irq_lend.ko",
        "drivers/virt/gunyah/gh_mem_notifier.ko",
        "drivers/virt/gunyah/gh_msgq.ko",
        "drivers/virt/gunyah/gh_rm_drv.ko",
        "drivers/virt/gunyah/gh_virt_wdt.ko",
        "drivers/virt/gunyah/gunyah_loader.ko",
        "kernel/sched/walt/sched-walt.ko",
        "kernel/trace/qcom_ipc_logging.ko",
        "net/mac80211/mac80211.ko",
        "net/qrtr/qrtr.ko",
        "net/qrtr/qrtr-mhi.ko",
        "net/qrtr/qrtr-smd.ko",
        "net/qrtr/qrtr-tun.ko",
        "net/wireless/cfg80211.ko",
    ]

    _pineapple_consolidate_in_tree_modules = _pineapple_in_tree_modules + [
        # keep sorted
        "drivers/misc/lkdtm/lkdtm.ko",
        "kernel/locking/locktorture.ko",
        "kernel/rcu/rcutorture.ko",
        "kernel/torture.ko",
        "lib/atomic64_test.ko",
        "lib/test_user_copy.ko",
    ]

    kernel_vendor_cmdline_extras = ["bootconfig"]

    for variant in la_variants:
        board_kernel_cmdline_extras = []
        board_bootconfig_extras = []

        if variant == "consolidate":
            mod_list = _pineapple_consolidate_in_tree_modules
            board_bootconfig_extras += ["androidboot.serialconsole=1"]
            board_kernel_cmdline_extras += [
                # do not sort
                "console=ttyMSM0,115200n8",
                "qcom_geni_serial.con_enabled=1",
                "earlycon",
            ]
            kernel_vendor_cmdline_extras += [
                # do not sort
                "console=ttyMSM0,115200n8",
                "qcom_geni_serial.con_enabled=1",
                "earlycon",
            ]
        else:
            mod_list = _pineapple_in_tree_modules
            board_kernel_cmdline_extras += ["nosoftlockup console=ttynull qcom_geni_serial.con_enabled=0"]
            kernel_vendor_cmdline_extras += ["nosoftlockup console=ttynull qcom_geni_serial.con_enabled=0"]
            board_bootconfig_extras += ["androidboot.serialconsole=0"]

        define_msm_la(
            msm_target = target_name,
            msm_arch = target_arch,
            variant = variant,
            in_tree_module_list = mod_list,
            boot_image_opts = boot_image_opts(
                earlycon_addr = "qcom_geni,0x0089C000",
                kernel_vendor_cmdline_extras = kernel_vendor_cmdline_extras,
                board_kernel_cmdline_extras = board_kernel_cmdline_extras,
                board_bootconfig_extras = board_bootconfig_extras,
            ),
        )
