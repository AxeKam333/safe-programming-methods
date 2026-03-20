# safe-programming-methods

## sesja interaktywna
srun -A inf155840 -p pmem --pty /bin/bash -l
(i potem na klastrze gcc -Wall ...)

srun -p pmem -c32 --pty ./a.out

## batch job
sbatch batch.sh

wyłączyć optymalizacje
na pmem do 96 wątkach
goldbolt.org - jak kod został skompilowany