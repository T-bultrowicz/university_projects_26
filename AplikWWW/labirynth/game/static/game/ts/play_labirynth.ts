import { api, Result } from "./api";

const timer = document.getElementById('timer') as HTMLDivElement | null;
const penaltyTimer = document.getElementById('penalty-timer') as HTMLDivElement | null;
const leaderboard = document.getElementById('leaderboard') as HTMLDivElement | null;
const mazeContainer = document.getElementById('maze-container') as HTMLDivElement | null;
const canvas = document.getElementById('maze-canvas') as HTMLCanvasElement | null;
const errorLog = document.getElementById('errors') as HTMLHeadingElement | null;
const mazeId = mazeContainer?.getAttribute('data-id') || null;

const playerSpriteImg = new Image();
playerSpriteImg.src = '/static/game/img/playerSpriteImg.png';
const redFlgSpriteImg = new Image();
redFlgSpriteImg.src = '/static/game/img/redFlagSprite.png';
const finishSpriteImg = new Image();
finishSpriteImg.src = '/static/game/img/finishSprite.png';
const bushesSpriteImg = new Image();
bushesSpriteImg.src = '/static/game/img/bushesSprite.png';

interface Point { x: number; y: number; }
interface AppState {
    maze: number[][] | null;
    mazeName: string;
    playerPosition: Point;
    playerVisualPosition: Point;
    lastFlagPosition: Point;
    flagsPosition: Point[];
    visitedCorners: Point[];
    penaltyTime: number;
    startTime: number;
    tileSize: number;
    playerSpeed: number;
    lastFrameTime: number;
    onPenalty: number | null;
}

let state: AppState = {
    maze: null,
    mazeName: "",
    playerPosition: { x: 0, y: 0 },
    playerVisualPosition: { x: 0, y: 0 },
    lastFlagPosition: { x: 0, y: 0 },
    flagsPosition: [],
    visitedCorners: [],
    penaltyTime: 0,
    startTime: 0,
    tileSize: 0,
    playerSpeed: 4.5,
    lastFrameTime: 0,
    onPenalty: 0,
};

const BITS = { UP: 1, RIGHT: 2, DOWN: 4, LEFT: 8, START: 16, END: 32, DENSE: 64 };
const isUpBound = (x: number) => (x & BITS.UP) !== 0;
const isRightBound = (x: number) => (x & BITS.RIGHT) !== 0;
const isDownBound = (x: number) => (x & BITS.DOWN) !== 0;
const isLeftBound = (x: number) => (x & BITS.LEFT) !== 0;
const isStart = (x: number) => (x & BITS.START) !== 0;
const isEnd = (x: number) => (x & BITS.END) !== 0;
const isDense = (x: number) => (x & BITS.DENSE) !== 0;

function render() {
    if (!canvas || !state.maze) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;
    if (timer)
        timer.innerHTML = `<b> Time: </b> ${((Date.now() - state.startTime) / 1000).toFixed(3)}s`;
    if (penaltyTimer)
        penaltyTimer.innerHTML = `<b> Penalty Time: </b> ${state.penaltyTime.toFixed(3)}s`;

    const rows = state.maze.length;
    const cols = state.maze[0].length;
    const ts = state.tileSize;

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const cell = state.maze[r][c];
            const x = c * ts;
            const y = r * ts;

            if (isStart(cell)){
                ctx.fillStyle = 'green';
                ctx.fillRect(x, y, ts, ts);
            } 
            else if (isEnd(cell)) {
                ctx.drawImage(finishSpriteImg, x, y, ts, ts);
            }
            else if (isDense(cell)) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x, y, ts, ts);
                ctx.drawImage(bushesSpriteImg, x, y, ts, ts);
            }
            else {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x, y, ts, ts);
            }

            // Draw grid lines
            ctx.strokeStyle = 'rgba(0,0,0,0.2)';
            ctx.lineWidth = Math.max(1, Math.floor(ts * 0.025));
            ctx.strokeRect(x, y, ts, ts);

            // Draw solid walls
            ctx.strokeStyle = 'black';
            ctx.lineWidth = Math.max(2, Math.floor(ts * 0.075));
            ctx.beginPath();
            
            if (isUpBound(cell)) { ctx.moveTo(x, y); ctx.lineTo(x + ts, y); }
            if (isRightBound(cell)) { ctx.moveTo(x + ts, y); ctx.lineTo(x + ts, y + ts); }
            if (isDownBound(cell)) { ctx.moveTo(x, y + ts); ctx.lineTo(x + ts, y + ts); }
            if (isLeftBound(cell)) { ctx.moveTo(x, y); ctx.lineTo(x, y + ts); }
            
            ctx.stroke();
        }
    }

    // Drawing history (red)
    if (state.visitedCorners.length > 0) {
        ctx.strokeStyle = 'rgba(255, 0, 0, 0.6)';
        ctx.lineWidth = Math.max(2, Math.floor(ts * 0.15));
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        ctx.beginPath();
        ctx.moveTo(state.visitedCorners[0].x * ts + ts / 2, state.visitedCorners[0].y * ts + ts / 2);

        for (let i = 1; i < state.visitedCorners.length; i++) {
            ctx.lineTo(state.visitedCorners[i].x * ts + ts / 2, state.visitedCorners[i].y * ts + ts / 2);
        }

        ctx.lineTo(state.playerVisualPosition.x * ts + ts / 2, state.playerVisualPosition.y * ts + ts / 2);
        ctx.stroke();
    }

    // Drawing planned route (blue)
    if (state.flagsPosition.length > 0) {
        ctx.strokeStyle = 'rgba(0, 0, 255, 0.6)';
        ctx.lineWidth = Math.max(2, Math.floor(ts * 0.15));
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';

        ctx.beginPath();
        ctx.moveTo(state.playerVisualPosition.x * ts + ts / 2, state.playerVisualPosition.y * ts + ts / 2);
        
        for (const flag of state.flagsPosition) {
            ctx.lineTo(flag.x * ts + ts / 2, flag.y * ts + ts / 2);
        }
        ctx.stroke();
    }

    // Draw Flags
    for (const flag of state.flagsPosition) {
        ctx.drawImage(
            redFlgSpriteImg, 
            flag.x * ts + (ts / 4), 
            flag.y * ts + (ts / 4), 
            ts / 2, 
            ts / 2
        );
    }
    // Draw Player
    ctx.drawImage(
        playerSpriteImg, 
        state.playerVisualPosition.x * ts + (ts / 6), 
        state.playerVisualPosition.y * ts + (ts / 6), 
        ts * 2 / 3, 
        ts * 2 / 3
    );
}

function initializeMaze() {
    if (!canvas || !state.maze) return;

    const rows = state.maze.length;
    const cols = state.maze[0].length;
    if (rows === 0) return;

    const maxWidth = mazeContainer?.clientWidth || (window.innerWidth * 0.9);
    const maxHeight = window.innerHeight * 0.75;
    console.log(`Container size: ${maxWidth}x${maxHeight}, Maze size: ${cols}x${rows}`);
    state.tileSize = Math.max(10, Math.min(Math.floor(maxWidth / cols), Math.floor(maxHeight / rows)));

    canvas.width = cols * state.tileSize;
    canvas.height = rows * state.tileSize;
    // Find start position
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            if (isStart(state.maze[r][c])) {
                state.playerPosition = { x: c, y: r };
                state.playerVisualPosition = { x: c, y: r };
                state.lastFlagPosition = { x: c, y: r };
                state.visitedCorners = [{ x: c, y: r }];
            }
        }
    }

    render();
}

async function init() {
    const result = await api.getMaze(Number(mazeId));
    if (result.ok) {
        if (errorLog) errorLog.textContent = "";
        state.penaltyTime = 0;
        state.maze = result.data.structure;
        state.mazeName = result.data.name;
        state.startTime = Date.now();
        state.onPenalty = Date.now();
        initializeMaze();

        requestAnimationFrame(gameLoop);
    } else {
        if (errorLog) errorLog.textContent = "Failed to load maze data, waiting for connection...";
        setTimeout(init, 2000);
    }
}

function update(dt: number) {
    if (state.flagsPosition.length === 0) {
        if (state.onPenalty) {
            state.penaltyTime += (Date.now() - state.onPenalty) / 1000;
            state.onPenalty = Date.now();
        }
        return;
    }

    const target = state.flagsPosition[0]; // Cel to zawsze pierwsza flaga
    let moveStep = state.playerSpeed * dt; // Ile kratek pokona w tej klatce
    const epsilon = 0.05; // Tolerancja dla ułamków (kiedy uznajemy, że dotarł do celu w danej osi)

    let vx = state.playerVisualPosition.x;
    let vy = state.playerVisualPosition.y;
    const currentX = Math.round(vx);
    const currentY = Math.round(vy);
    if (state.maze && state.maze[currentY][currentX]) {
        const cell = state.maze[currentY][currentX];
        if (isDense(cell)) {
            moveStep /= 5;
        }
    }


    if (Math.abs(target.x - vx) > epsilon) {
        if (vx < target.x) vx += moveStep;
        else vx -= moveStep;
    } 
    else if (Math.abs(target.y - vy) > epsilon) {
        vx = target.x;
        if (vy < target.y) vy += moveStep;
        else vy -= moveStep;
    } 
    else { // Duszek dotarł do flagi.
        vx = target.x;
        vy = target.y;
        state.visitedCorners.push({ x: target.x, y: target.y });
        state.flagsPosition.shift(); // Usuwamy zdobytą flagę
        if (state.flagsPosition.length === 0) {
            state.onPenalty = Date.now();
        }
    }

    state.playerVisualPosition.x = vx;
    state.playerVisualPosition.y = vy;

    state.playerPosition.x = Math.round(vx);
    state.playerPosition.y = Math.round(vy);
}

async function gameLoop(timestamp: number): Promise<void> {
    if (!state.lastFrameTime) state.lastFrameTime = timestamp;
    
    const dt = (timestamp - state.lastFrameTime) / 1000;
    state.lastFrameTime = timestamp;

    const safeDt = Math.min(dt, 0.1);
    update(safeDt);
    render();
    if (isEnd(state.maze![state.playerPosition.y][state.playerPosition.x])) {
        const resultTime = state.penaltyTime + ((Date.now() - state.startTime) / 1000);
        const sendTime = Math.round(resultTime * 1000); // całkowita liczba w ms
        timer!.innerHTML = "";
        penaltyTimer!.innerHTML = "";
        mazeContainer!.innerHTML = "";
        const result = await api.submitRun(Number(mazeId), sendTime);
        if (!result.ok) {
            if (errorLog) errorLog.textContent = "Failed to submit run due to error: " + result.error;
            return;
        }
        if (errorLog) 
            errorLog.textContent = 
            `Congratulations - you finished ${state.mazeName}! Your time: ${resultTime.toFixed(3)}s`;
    } else {
        // Zapętl
        requestAnimationFrame(gameLoop);
    }
}

function canAddPosition(x: number, y: number): Result<void> {
    const xNow = state.lastFlagPosition.x;
    const yNow = state.lastFlagPosition.y;

    if (x !== xNow && y !== yNow) {
        return { ok: false, error: "You can only place flags in straight lines!" };
    }
    if (x === xNow && y === yNow) {
        return { ok: false, error: "You are already here!" };
    }
    const maze = state.maze!;
    if (x === xNow) {
        const yMin = Math.min(yNow, y);
        const yMax = Math.max(yNow, y);

        for (let i = yMin; i < yMax; i++) {
            if (isDownBound(maze[i][x]) || isUpBound(maze[i + 1][x])) {
                return { ok: false, error: "You can't place flags through walls!" };
            }
        }
    }
    else if (y === yNow) {
        const xMin = Math.min(xNow, x);
        const xMax = Math.max(xNow, x);

        for (let i = xMin; i < xMax; i++) {
            if (isRightBound(maze[y][i]) || isLeftBound(maze[y][i + 1])) {
                return { ok: false, error: "You can't place flags through walls!" };
            }
        }
    }

    return { ok: true, data: undefined };
}

function clearErrorAfterDelay() {
    setTimeout(() => {
        if (errorLog) errorLog.textContent = "";
    }, 1500);
}

if (canvas) {
    canvas.addEventListener('click', (e: MouseEvent) => {
        if (!state.maze || !canvas) return;
        // 1. Obliczamy współczynnik skali (jeśli CSS np. trochę pomniejszył canvas)
        const scaleX = canvas.width / canvas.clientWidth;
        const scaleY = canvas.height / canvas.clientHeight;

        // 2. Mnożymy pozycję myszy przez skalę, aby uzyskać prawdziwe piksele wewnątrz canvasu
        const internalX = e.offsetX * scaleX;
        const internalY = e.offsetY * scaleY;

        // 3. Dzielimy przez wielkość kratki i zaokrąglamy w dół, by dostać indeks
        const cellX = Math.floor(internalX / state.tileSize);
        const cellY = Math.floor(internalY / state.tileSize);

        // Dodajemy cel dla duszka
        const result = canAddPosition(cellX, cellY);
        if (result.ok) {
            const newFlag = { x: cellX, y: cellY };
            state.flagsPosition.push(newFlag);
            state.onPenalty = null;
            state.lastFlagPosition = newFlag;
        } else {
            if (errorLog) errorLog.textContent = result.error;
            clearErrorAfterDelay();
        }
    });
}

interface StreamedRun {
    player: string;
    time: number;
}

if (leaderboard) {
    const evtSource = new EventSource(`/leaderboard_stream/?level_id=${mazeId}`);

    evtSource.onmessage = (event: MessageEvent) => {
        try {
            const topRuns: StreamedRun[] = JSON.parse(event.data);
            if (topRuns.length === 0) {
                leaderboard.innerHTML = `<p class="no-runs">No recorded runs for this level yet. Be the first!</p>`;
                return;
            }

            // 2. Build a cleanly structured HTML table for the records
            let tableHTML = `
                <h3 class="leaderboard-title">Top 5 Runs</h3>
                <table class="leaderboard-table">
                    <thead>
                        <tr>
                            <th>Rank</th>
                            <th>Player</th>
                            <th>Time</th>
                        </tr>
                    </thead>
                    <tbody>
            `;
            topRuns.forEach((run, index) => {
                const runTimeSeconds = (run.time / 1000).toFixed(3);
                tableHTML += `
                    <tr class="rank-${index + 1}">
                        <td><strong>#${index + 1}</strong></td>
                        <td><span class="player-name">${run.player}</span></td>
                        <td><span class="player-time">${runTimeSeconds}s</span></td>
                    </tr>
                `;
            });
            tableHTML += `
                    </tbody>
                </table>
            `;
            leaderboard.innerHTML = tableHTML;

        } catch (error) {
            leaderboard.innerHTML = `<p class="errors"> Failed to sync live leaderboard metrics.</p>`;
        }
    };

    evtSource.onerror = () => {
        leaderboard.innerHTML = `
            <p class="errors">
                Connection lost. Attempting to reconnect live leaderboard...
            </p>
        `;
    };
}

init();