function delay(ms: number): Promise<void> {
    return new Promise((resolve) => setTimeout(resolve, ms));
}

// async/await makes Promise code read top-to-bottom:
async function countdown(element: HTMLElement): Promise<void> {
    for (let i = 5; i >= 0; i--) {
        element.textContent = `${i}…`;
        await delay(1000);
    }
    element.textContent = "Go!";
}

interface Todo {
    userId:    number;
    id:        number;
    title:     string;
    completed: boolean;
}

async function fetchTodo(id: number): Promise<Todo> {
    const res = await fetch(`https://jsonplaceholder.typicode.com/todos/${id}`);
    if (!res.ok) {
        throw new Error(`HTTP ${res.status}`);
    }
    const data: Todo = await res.json();
    return data;
}

async function fetchWithTimeout<T>(url:string, ms:number): Promise<T> {
    const timeout = new Promise<never>((_, reject) => 
        delay(ms).then(() => reject(new Error("timeout"))))
    const fetchPromise = fetch(url).then(async (res) => {
        if (!res.ok) {
            throw new Error(`HTTP ${res.status}`);
        }
        const data: Promise<T> = res.json();
        return data;
    })
    return Promise.race([timeout, fetchPromise]);
}

export { delay, countdown, fetchTodo, Todo, fetchWithTimeout };