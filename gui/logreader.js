const fs = require('fs');
const readline = require('readline');

const inputFile = 'debug_log.txt';

const threadFiles = {};

const rl = readline.createInterface({
    input: fs.createReadStream(inputFile),
    output: process.stdout,
    terminal: false
});

rl.on('line', (line) => {
    const match = line.match(/TH#(\d+):/);
    if (match) {
        const threadNum = match[1];
        
        if (!threadFiles[threadNum]) {
            const filename = `logs/thread_${threadNum}.log`;
            threadFiles[threadNum] = fs.createWriteStream(filename, { flags: 'a' });
            console.log(`Создан файл для потока ${threadNum}: ${filename}`);
        }
        
        threadFiles[threadNum].write(line + '\n');
    }
});

rl.on('close', () => {
    Object.values(threadFiles).forEach(writeStream => {
        writeStream.end();
    });
    console.log('Разделение логов завершено');
});