type Result<T> =
    | { ok: true;  data: T }
    | { ok: false; error: string; status?: number };

interface Maze {
    id:        number;
    name:      string;
    structure: number[][];
}

interface Run {
    id:     number;
    user:   string;
    maze:   Maze;
    time:   number; // in ms
    date:   string; // ISO format
}

function getCsrfToken(): string {
    const match = document.cookie.match(/csrftoken=([^;]+)/);
    return match ? match[1] : "";
}

async function apiFetch<T>(
    method: string,
    path: string,
    body?: unknown
): Promise<Result<T>> {
    try {
        const options: RequestInit = {
            method,
            headers: {
                "Content-Type": "application/json",
                "X-CSRFToken":  getCsrfToken(),
            },
        };
        if (body !== undefined) {
            options.body = JSON.stringify(body);
        }

        const res = await fetch(path, options);

        if (!res.ok) {
            let message = `HTTP ${res.status}`;
            try {
                const err = await res.json();
                message = err.error ?? message;
            } catch { /* ignore */ }
            return { ok: false, error: message, status: res.status };
        }

        if (res.status === 204) {
            return { ok: true, data: {} as T };
        }

        const data: T = await res.json();
        return { ok: true, data };

    } catch (err) {
        return { ok: false, error: String(err) };
    }
}

class GameApiClient {
    // TODO: implement searching mechanism for mazes and runs, 
    // by modyfing getmazes, getruns and doing it in django backend.
    async getMazes(query: string = ""): Promise<Result<{ mazes: Maze[] }>> {
        const url = (query.trim() !== "")
            ? `/api/mazes/?q=${encodeURIComponent(query.trim())}` 
            : `/api/mazes/`;
        
        return apiFetch("GET", url);
    }

    async getMaze(id: number): Promise<Result<Maze>> {
        return apiFetch("GET", `/api/mazes/${id}/`);
    }

    async getRuns(mazeId?: number): Promise<Result<{ runs: Run[] }>> {
        const url = mazeId ? `/api/mazes/${mazeId}/runs/` : '/api/runs/';
        return apiFetch("GET", url);
    }

    async getRun(id: number): Promise<Result<Run>> {
        return apiFetch("GET", `/api/runs/${id}/`);
    }

    async submitRun(mazeId: number, time: number): Promise<Result<Run>> {
        return apiFetch("POST", `/api/mazes/${mazeId}/runs/`, { "time": time });
    }
}

async function withRetry<T>(
    fn: () => Promise<Result<T>>,
    retries: number,
    delayMs: number
): Promise<Result<T>> {
    let lastError: Result<T> | null = null;
    let currentDelay = delayMs;
    for (let attempt = 0; attempt <= retries; attempt++) {
        console.log(`Attempt ${attempt + 1}/${retries + 1}...`);

        const result = await fn();
        if (result.ok) {
            return result;
        }
        lastError = result;
        if (attempt < retries) {
            console.log(`Retry ${attempt + 1}/${retries} (waiting ${currentDelay}ms)...`);
            await new Promise(res => setTimeout(res, currentDelay));
            currentDelay *= 2;
        }
    }

    return lastError ?? { ok: false, error: "Unknown error" };
}

type ValidationResult =
    | { valid: true }
    | { valid: false; errors: Record<string, string> };

const api = new GameApiClient();

export { api, GameApiClient, withRetry, Maze, Run, Result, ValidationResult};