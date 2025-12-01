const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const { spawn } = require('child_process');
const path = require('path');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

// Serve static files from 'public' directory
app.use(express.static(path.join(__dirname, 'public')));

let gameProcess = null;
let currentScore = { p1: 3, p2: 3 };

// SOCKET.IO CONNECTION
io.on('connection', (socket) => {
    console.log('Web client connected');
    // Send current status
    socket.emit('score_update', currentScore);
    socket.emit('status', gameProcess ? 'running' : 'stopped');

    // START GAME COMMAND 
    socket.on('start_game', () => {
        if (gameProcess) return; // Already running

        console.log('Starting BattleBots C Engine...');
        
        // Reset Score
        currentScore = { p1: 3, p2: 3 };
        io.emit('score_update', currentScore);
        io.emit('status', 'running');

        // SPAWN THE C PROGRAM (need sudo)
        gameProcess = spawn('sudo', ['./BattleBots']);

        // Listen to C program output (printf)
        gameProcess.stdout.on('data', (data) => {
            const output = data.toString();
            console.log(`[C-Engine]: ${output}`); 

            parseOutput(output);
        });

        gameProcess.stderr.on('data', (data) => {
            console.error(`[C-Error]: ${data}`);
        });

        gameProcess.on('close', (code) => {
            console.log(`BattleBots process exited with code ${code}`);
            gameProcess = null;
            io.emit('status', 'stopped');
        });
    });

    // STOP GAME COMMAND 
    socket.on('stop_game', () => {
        if (gameProcess) {
            console.log('Stopping BattleBots...');
            // We need to use sudo kill because we started with sudo
            spawn('sudo', ['kill', '-SIGINT', gameProcess.pid]);
            gameProcess = null;
            io.emit('status', 'stopped');
        }
    });
});

// PARSER LOGIC 
// Reads your printf statements from main.c
function parseOutput(text) {
    // Expected C output: "Bot 1 was hit! Lives remaining: 2"
    
    // Check for Bot 1 hit
    if (text.includes("Bot 1 was hit")) {
        // Extract number 
        const match = text.match(/Lives remaining: (\d+)/);
        if (match) {
            currentScore.p1 = parseInt(match[1]);
            io.emit('score_update', currentScore);
        }
    }
    
    // Check for Bot 2 hit
    if (text.includes("Bot 2 was hit")) {
        const match = text.match(/Lives remaining: (\d+)/);
        if (match) {
            currentScore.p2 = parseInt(match[1]);
            io.emit('score_update', currentScore);
        }
    }

    // Check for Game Over
    if (text.includes("Game over")) {
        let winner = "Draw";
        if (text.includes("Bot 1 wins")) winner = "Player 1 Wins!";
        if (text.includes("Bot 2 wins")) winner = "Player 2 Wins!";
        
        io.emit('game_over', winner);
    }
}

server.listen(8080, () => {
    console.log('BattleBots Web Server running on port 8080');
});