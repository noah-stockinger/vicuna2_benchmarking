###
# Set configurations of the system here.  This is imported into CMAKE
###

#Currently Supported: rv32im, rv32im_zve32x, rv32imf, rv32imf_zhf, rv32imf_zve32x, rv32imf_zve32f, rv32im_zve32x_zvbb, rv32im_zve32x_zvbc, rv32im_zve32x_zvbb_zvbc
set(RISCV_ARCH "rv32im_zve32x")

#Currently Supported: cv32e40x, cv32a60x
set(SCALAR_CORE "cv32a60x")

set(VMEM_PORTS 1)
set(VMEM_W 32)
set(VREG_W 128)

math(EXPR VMEM_TOTAL_W "${VMEM_PORTS} * ${VMEM_W}")

set(VPROC_PIPELINES "${VMEM_TOTAL_W}:VLSU 32:VXRESULT,VSLD,VDIV,VALU,VMUL,VREDSUM,VREDMINMAX")

