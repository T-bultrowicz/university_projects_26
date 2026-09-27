import { delay, countdown, fetchTodo, Todo, fetchWithTimeout } from "./async"

// A helper to display results in the page instead of the console.
function show(label: string, value: unknown): void {
    const output = document.getElementById("output")!;
    const line = document.createElement("p");
    line.textContent = `${label}: ${String(value)}`;
    output.appendChild(line);
}

// TODO: Write a function greet(name: string, times: number): string
// that returns "Hello, <name>! ".repeat(times).trim()
// Try calling it with greet("Alice", "3") — TypeScript should error.

function greet(name: string, times: number): string {
    return `Hello, ${name}! `.repeat(times).trim();
}

// TODO: Write a function clamp(value: number, min: number, max: number): number
// that returns value clamped to [min, max].
function clamp(value: number, min: number, max: number): number {
    if (value < min)
        return min;
    else if (value > max)
        return max;
    return value;
}

// TODO: Write a function formatDuration(totalSeconds: number): string
// that converts seconds to a human-readable string.
// Examples:
//   formatDuration(0)     → "0s"
//   formatDuration(62)    → "1m 2s"
//   formatDuration(3661)  → "1h 1m 1s"
//   formatDuration(86400) → "24h 0m 0s"
function formatDuration(totalSeconds: number): string {
    if (totalSeconds < 60)
        return `${totalSeconds}s`;
    else if (totalSeconds < 3600)
        return `${Math.trunc(totalSeconds / 60)}m ${totalSeconds % 60}s`;
    return `${Math.trunc(totalSeconds / 3600)}h ${Math.trunc(totalSeconds / 60) % 60}m ${totalSeconds% 60}s`;
}

// Render a table of test cases in the page:
const testCases: Array<[number, string]> = [
    [0, "0s"], [5, "5s"], [62, "1m 2s"],
    [3661, "1h 1m 1s"], [86400, "24h 0m 0s"],
];

const table = document.createElement("table");
table.innerHTML = "<tr><th>Input</th><th>Expected</th><th>Got</th><th>✓</th></tr>";
for (const [input, expected] of testCases) {
    const got = formatDuration(input);
    const pass = got === expected;
    const row = document.createElement("tr");
    row.innerHTML = `<td>${input}</td><td>${expected}</td><td>${got}</td>
                     <td>${pass ? "✅" : "❌"}</td>`;
    if (!pass) row.classList.add("error");
    table.appendChild(row);
}
document.getElementById("output")!.appendChild(table);


show("greet", greet("World", 2));
show("clamp(15, 0, 10)", clamp(15, 0, 10));   // → 10
show("clamp(-5, 0, 10)", clamp(-5, 0, 10));   // → 0
show("clamp(-5, -11, 10)", clamp(-5, -11, 10));   // → -5

const counter = document.createElement("h2");
document.getElementById("output")!.appendChild(counter);
countdown(counter);

async function renderTodos(): Promise<void> {
    const container = document.createElement("div");
    container.innerHTML = "<h3>Todos from API</h3>";
    document.getElementById("output")!.appendChild(container);

    for (const id of [1, 2, 3]) {
        const todo = await fetchTodo(id);
        const p = document.createElement("p");
        p.textContent = `${todo.completed ? "✅" : "⬜"} ${todo.title}`;
        container.appendChild(p);
    }
}
renderTodos();

function showTestResult(promise: Promise<any>): void {
    const container = document.createElement("div");
    const p = document.createElement("p");
    promise.then(
        () => {p.textContent = `✅ Succeeded`}, () => {p.textContent = `❌ timed out`}
    )
    container.appendChild(p);
    document.getElementById("output")!.appendChild(container);
}

function fetchParallel() {
    const container = document.createElement("div");
    let promises = [];
    let start = performance.now();
    for (let i = 1; i < 11; i++) {
        promises[i - 1] = fetchTodo(i);
    }
    Promise.all(promises).then((val) => {
        for (let i = 0; i < promises.length; ++i) {
            const p = document.createElement("p");
            p.textContent = `${val[i].completed ? "✅" : "⬜"} ${val[i].title}`;
            container.appendChild(p);
        }
    }, () => {throw new Error("Not worked") })

    document.getElementById("output")!.appendChild(container);
    let elapsed = performance.now() - start;
    console.log(elapsed);
}

function fetchSequence() {
    const container = document.createElement("div");
    let start = performance.now();
    for (let i = 1; i < 11; i++) {
        fetchTodo(i).then((val) => {
            const p = document.createElement("p");
            p.textContent = `${val.completed ? "✅" : "⬜"} ${val.title}`;
            container.appendChild(p);
        }, () => {throw new Error("Not worked") })
    }
    document.getElementById("output")!.appendChild(container);
    let elapsed = performance.now() - start;
    console.log(elapsed);
}   

let url = "https://jsonplaceholder.typicode.com/todos/1"
showTestResult(fetchWithTimeout<Todo>(url, 1));
showTestResult(fetchWithTimeout<Todo>(url, 5000));
fetchParallel();
fetchSequence();

import { Post, summarise, filterByCategory, sortPosts, validatePost } from "./blog";

const posts: Post[] = [
    {
        id: 1, title: "Hello TypeScript", slug: "hello-ts",
        body: "TypeScript is JavaScript with types. It compiles to plain JS.",
        pubDate: "2025-01-01",
        category: { id: 1, name: "Tech", slug: "tech" },
    },
    {
        id: 2, title: "CSS Grid", slug: "css-grid",
        body: "CSS Grid is a two-dimensional layout system for the web.",
        pubDate: "2025-01-15",
        category: { id: 2, name: "Frontend", slug: "frontend" },
    },
    {
        id: 3, title: "Django REST", slug: "django-rest",
        body: "Build a REST API with Django and serve JSON to any client.",
        pubDate: "2025-02-01",
        category: { id: 1, name: "Tech", slug: "tech" },
    },
];

// Render each post as a card
function renderPostCard(post: Post): HTMLElement {
    const card = document.createElement("article");
    card.innerHTML = `<h3>${post.title}</h3><p>${summarise(post)}</p>`;
    return card;
}

const output = document.getElementById("output")!;
const select = document.createElement("select");
let sortedPosts = posts;
select.innerHTML = `
    <option value="title">Title</option>
    <option value="date">Date</option>
    <option value="category">Category</option>
`;
select.addEventListener("change", () => {
    sortedPosts = sortPosts(posts, select.value as "title" | "date" | "category");
    postsSection.innerHTML = `<h2>All posts sorted by ${select.value} </h2>`;
    for (const post of sortedPosts) {
        postsSection.appendChild(renderPostCard(post));
    }
});

output.appendChild(select);
const postsSection = document.createElement("div");
postsSection.innerHTML = "<h2>All posts</h2>";
output.appendChild(postsSection);
for (const post of sortedPosts) {
    postsSection.appendChild(renderPostCard(post));
}

// Show filtered results
const techPosts = filterByCategory(posts, "tech");
const filteredSection = document.createElement("div");
filteredSection.innerHTML = `<h3>Tech posts: ${techPosts.map(p => p.title).join(", ")}</h3>`;
output.appendChild(filteredSection);

import { Shape, area, renderShape } from "./shapes";
import { groupBy, memoize, pipe } from "./utils";

const shapes: Shape[] = [
    { kind: "circle", radius: 20 },
    { kind: "rectangle", width: 80, height: 80 },
    { kind: "triangle", base: 10, height: 60 },
];

const shapesSection = document.createElement("div");
shapesSection.innerHTML = "<h2>Shapes</h2>";
for (const shape of shapes) {
    const el = renderShape(shape);

    const a = area(shape);
    const label = document.createElement("p");
    label.textContent = `Area: ${a.toFixed(2)}`;

    shapesSection.appendChild(el);
    shapesSection.appendChild(label);
}
output.appendChild(shapesSection);

const validPostSection = document.createElement("div");
const textarea = document.createElement("textarea");
textarea.placeholder = "Enter post data as JSON";
const validateButton = document.createElement("button");
validateButton.textContent = "Validate post";
const validationResult = document.createElement("p");

validPostSection.appendChild(textarea);
validPostSection.appendChild(validateButton);
validPostSection.appendChild(validationResult);

validateButton.addEventListener("click", () => {
    validationResult.textContent = "";

    const data = JSON.parse(textarea.value);
    const result = validatePost(data);
    if (result.ok) {
        validationResult.textContent = `✅ Valid post: ${result.post.title}`;
    } else {
        validationResult.textContent = `❌ Invalid post: ${result.reason}`;
    }
});
output.appendChild(validPostSection);

const groupedSection = document.createElement("div");
const groupedPosts = groupBy(posts, post => post.category?.slug ?? "uncategorised");
for (const [key, value] of Object.entries(groupedPosts)) {
    const categorySection = document.createElement("div");
    categorySection.innerHTML = `<h3>${key}:</h3>`;
    for (const post of value) {
        categorySection.appendChild(renderPostCard(post));
    }
    groupedSection.appendChild(categorySection);
}
output.appendChild(groupedSection);

const process = pipe<Post[]>(
    posts => posts.filter(p => p.category !== null),
    posts => posts.filter(p => p.pubDate >= "2025-01-10"),
    posts => posts.sort((a, b) => a.title.localeCompare(b.title)),
    posts => posts.slice(0, 5),
);
const pipeSection = document.createElement("div");
pipeSection.innerHTML = "<h2>Processed posts</h2>";
for (const post of process(posts)) {
    pipeSection.appendChild(renderPostCard(post));
}
output.appendChild(pipeSection);

const memoizeSection = document.createElement("div");
memoizeSection.innerHTML = "<h2>Memoized fetch</h2>";
const memoizedFetch = memoize(fetchTodo);

const testFetch = async (id: number) => {
    const start = performance.now();
    const todo = await memoizedFetch(id);
    const elapsed = performance.now() - start;

    const p = document.createElement("p");
    p.textContent = `Fetched: ${todo.title}, Cache ${elapsed < 100 ? "HIT" : "MISS"} (${elapsed.toFixed(2)}ms)`;
    memoizeSection.appendChild(p);
};

(async () => {
    await testFetch(1);
    await testFetch(1);
})();
output.appendChild(memoizeSection);