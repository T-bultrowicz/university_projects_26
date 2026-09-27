import os

with open('inputtt.txt', 'w') as dir_file:
    FROM_SET = []
    files = []
    for file in os.listdir('training_samples'):
        file = file.strip()
        if (len(FROM_SET) != 0) and (not any(file.startswith(prefix) for prefix in FROM_SET)):
            continue
        files.append(file)

    files.sort()
    for file in files:
        dir_file.write(os.path.join('training_samples', file) + '\n')
