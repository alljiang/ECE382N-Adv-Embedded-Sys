rsync koopa:/misc/scratch/ajiang1/BASELINE_SP_2024/ultra96v2_oob.runs/impl_1/ultra96v2_oob_wrapper.bit ./system.bit
rsync system.bit ultra96:~/system.bit

ssh -t ultra96 "sudo fpgautil -b ~/system.bit"
