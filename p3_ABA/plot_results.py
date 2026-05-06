import matplotlib.pyplot as plt

# Read the file
with open('wyniki/wyniki_telefon.txt', 'r') as f:
    lines = f.readlines()

# Parse data
threads = []
mops = []
for line in lines[2:]:  # Skip the header lines
    parts = line.split('|')
    if len(parts) >= 2:
        thread_part = parts[0].strip()
        mops_part = parts[1].strip()
        thread_num = int(thread_part.split(':')[1].strip())
        mops_val = float(mops_part.split('->')[1].strip().split()[0])
        threads.append(thread_num)
        mops.append(mops_val)

# Plot the data
plt.plot(threads, mops, marker='o', linestyle='-')
plt.xlabel('Number of Threads')
plt.ylabel('MOPS')
plt.title('LL/SC STACK Performance on ARM')
plt.grid(True)
plt.ylim(bottom=0)
plt.savefig('performance_plot.png')
print("Wykres został zapisany jako performance_plot.png")