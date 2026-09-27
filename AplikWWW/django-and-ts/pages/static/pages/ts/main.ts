import { Post, Result, api, withRetry, validateComment } from "./api";

// A helper that throws if the element is missing —
// better than silently failing at runtime.
function getElement<T extends HTMLElement>(id: string): T {
    const el = document.getElementById(id);
    if (el === null) {
        throw new Error(`Element #${id} not found`);
    }
    return el as T;
}

const searchInput = getElement<HTMLInputElement>("search-input");
const postList    = getElement<HTMLDivElement>("post-list");
const statusBar   = getElement<HTMLParagraphElement>("status-bar");
const sortSelect  = getElement<HTMLSelectElement>("sort-select");

type PageStatus = "idle" | "loading" | "success" | "error";
type SortField  = "title" | "date" | "category";

interface PostsResponse {
    posts: Post[];
}

interface AppState {
    posts:   Post[];
    query:   string;
    status:  PageStatus;
    error:   string;
    sortBy:  SortField;
    focusedIndex: number;
    activePolls: Map<number, ReturnType<typeof setInterval>>;
    visible: boolean;
}

interface PersistedState {
    query:  string;
    sortBy: SortField;
}

let state: AppState = {
    posts:  [],
    query:  "",
    status: "idle",
    error:  "",
    sortBy: "date",
    focusedIndex: -1,
    activePolls: new Map(),
    visible: true,
};

async function loadAndRender(query?: string): Promise<void> {

    const result = await api.getPosts(query);
    if (!result.ok) {
        statusBar.textContent = `Error: ${result.error}`;
        return;
    }

    let { posts } = result.data;
    posts = sortPosts(posts, state.sortBy);
    state.posts = posts;
    statusBar.textContent = `${posts.length} post(s)`;
    postList.innerHTML = "";
    for (let i = 0; i < posts.length; i++) {
        let rendered = renderPost(posts[i]);
        if (state.focusedIndex === i) {
            console.log("Focussed post ID:", i, "\n");
            rendered.classList.add("focused");
        }
        postList.appendChild(rendered);
    }
}

function highlightMatch(query: string, title: string): HTMLElement {
    let res = document.createElement("span");
    if (query === "") {
        res.innerHTML = title;
        return res;
    }
    query = query.replace(/[-\/\\^$*+?.()|[\]{}<>]/g, '\\$&'); // Escape regex special chars
    title = title.replace(new RegExp(`${query}`, "gi"), `<mark>${query}</mark>`);
    res.innerHTML = title;
    return res;
}

function savePrefs(prefs: PersistedState): void {
    localStorage.setItem("blog_prefs", JSON.stringify(prefs));
};

function isPersistedState(data: any): data is PersistedState {
    return typeof data === "object" &&
        typeof data.query === "string" && 
        (data.sortBy === "title" || data.sortBy === "date" || data.sortBy === "category");
}

function loadPrefs(): PersistedState | null {
    const raw = localStorage.getItem("blog_prefs");
    if (raw === null) {
        return null;
    }

    let parsed;
    try {
        parsed = JSON.parse(raw);
    } catch {
        return null;
    }
    return isPersistedState(parsed) ? parsed : null;
}

async function fetchPosts(query: string = ""): Promise<Post[]> {
    const url = query.length > 0
        ? `/api/posts/?search=${encodeURIComponent(query)}`
        : "/api/posts/";
    const res = await fetch(url);
    if (!res.ok) {
        throw new Error(`API error: ${res.status}`);
    }
    const data: PostsResponse = await res.json();
    return data.posts;
}

function sortPosts(posts: Post[], by: SortField): Post[] {
    const sorted = [...posts];
    switch(by) {
        case "title":
            sorted.sort((a, b) => a.title.localeCompare(b.title));
            break;
        case "date":
            sorted.sort((a, b) => new Date(b.pub_date).getTime() - new Date(a.pub_date).getTime());
            break;
        case "category":
            sorted.sort((a, b) => {
                if (a.category === null)
                    return 1;
                if (b.category === null)
                    return -1;
                return a.category.localeCompare(b.category);
            });
            break;
    }
    return sorted;
}

function renderPost(post: Post): HTMLElement {
    const article = document.createElement("article");

    const heading = document.createElement("h2");
    heading.appendChild(highlightMatch(state.query, post.title));

    const excerpt = document.createElement("p");
    excerpt.textContent = post.body.slice(0, 120) + "…";

    const fullBody = document.createElement("p");
    fullBody.textContent = post.body;
    fullBody.hidden = true;

    const comments = document.createElement("div");
    comments.classList.add("comments-container"); 
    comments.hidden = true;

    const lastUpdated = document.createElement("p");

    const toggle = document.createElement("button");
    toggle.textContent = "Read more";
    toggle.classList.add("toggle-btn");

    toggle.addEventListener("click", async () => {
        const isExpanded = !fullBody.hidden;
        fullBody.hidden = isExpanded;
        comments.hidden = isExpanded;
        lastUpdated.hidden = isExpanded;
        toggle.textContent = isExpanded ? "Read more" : "Show less";
        excerpt.hidden  = !isExpanded;

        if (!isExpanded) {
            renderComments(post, comments);
            startPolling(comments, lastUpdated, post);
        } else {
            stopPolling(post.id);
        }
    });

    article.addEventListener('keydown', (event) => {
        if (event.key === "Enter") {
            toggle.click();
        }
    });
    article.tabIndex = 0;

    article.append(heading, excerpt, fullBody, comments, lastUpdated, toggle);
    renderCommentForm(post.id, article); 
    return article;
}

async function renderComments(post: Post, commentContainer: HTMLElement): Promise<void> {
    commentContainer.innerHTML = "<p>Loading...</p>";
    const comms = await api.getComments(post.id);
    commentContainer.innerHTML = ""; 
    
    if (comms.ok) {
        for (const comment of comms.data.comments) {
            let div = document.createElement("div");
            div.innerHTML = `<strong>${comment.author}</strong>: ${comment.body}`;
            // Add optimistic delete button for each comment
            let delBtn = document.createElement("button");
            delBtn.textContent = "Delete";
            delBtn.style.marginLeft = "10px";
            delBtn.addEventListener("click", async () => {
                const originalContent = div.innerHTML;
                div.innerHTML = "Deleting...";
                const result = await api.deleteComment(post.id, comment.id);
                if (result.ok) {
                    div.remove();
                } else {
                    div.innerHTML = originalContent;
                    alert(`Error deleting comment: ${result.error}`);
                }
            });
            div.appendChild(delBtn);
            commentContainer.appendChild(div);
        }
    } else {
        commentContainer.textContent = `Error loading comments: ${comms.error}`;
    }
}

function renderCommentForm(postId: number, container: HTMLElement): void {
    const form = document.createElement("form");
    container.appendChild(form);

    const authorInput = document.createElement("input");
    authorInput.type        = "text";
    authorInput.placeholder = "Your name";
    authorInput.required    = true;

    const bodyTextarea = document.createElement("textarea");
    bodyTextarea.placeholder = "Your comment";
    bodyTextarea.required    = true;
    bodyTextarea.rows        = 3;

    const submitBtn = document.createElement("button");
    submitBtn.type        = "submit";
    submitBtn.textContent = "Post Comment";

    const feedback = document.createElement("p");

    form.append(authorInput, bodyTextarea, submitBtn, feedback);

    form.addEventListener("submit", async (event: Event) => {
        event.preventDefault();
        submitBtn.disabled = true;
        feedback.textContent = "";
        form.querySelectorAll(".error").forEach(el => el.remove());

        const validation = validateComment(authorInput.value, bodyTextarea.value);
        if (!validation.valid) {
            feedback.textContent = "Please fix the errors in the form.";
            Object.entries(validation.errors).forEach(([key, value]) => {
                let obj = document.createElement("span");
                obj.classList.add("error");
                obj.textContent = value;
                
                if ( key === "auth") {
                    authorInput.insertAdjacentElement("afterend", obj);
                } else if (key === "body") {
                    bodyTextarea.insertAdjacentElement("afterend", obj);
                }
            });
            submitBtn.disabled = false;
            return;
        }

        feedback.textContent = "Posting…";

        const result = await api.createComment(postId, {
            author: authorInput.value.trim(),
            body:   bodyTextarea.value.trim(),
        });

        if (result.ok) {
            feedback.textContent = "Comment posted!";
            form.reset();
            let div = document.createElement("div");
            div.innerHTML = `<strong>${result.data.author}</strong>: ${result.data.body}`;
            const commentsContainer = container.querySelector(".comments-container");
            if (commentsContainer) {
                commentsContainer.appendChild(div);
            }
            
        } else {
            feedback.textContent = `Error: ${result.error}`;
        }

        submitBtn.disabled = false;
    });
}

function render(): void {
    postList.innerHTML = "";
    if (state.status === "loading") {
        statusBar.textContent = "Loading…";
        return;
    }
    if (state.status === "error") {
        statusBar.textContent = `Error: ${state.error}`;
        return;
    }

    const sorted = sortPosts(state.posts, state.sortBy);
    statusBar.textContent = `${sorted.length} post(s) found`;
    for (let i = 0; i < sorted.length; i++) {
        let rendered = renderPost(sorted[i]);
        if (state.focusedIndex === i) {
            rendered.classList.add("focused");
        }
        postList.appendChild(rendered);
    }
}

// async function loadPosts(query: string = ""): Promise<void> {
//     state = { ...state, status: "loading", query };
//     render();

//     try {
//         const posts = await fetchPosts(query);
//         state = { ...state, posts, status: "success" };
//     } catch (err) {
//         state = { ...state, status: "error", error: String(err) };
//     }

//     render();
// }

let persisted = loadPrefs();
if (persisted !== null) {
    state = { ...state, query: persisted.query, sortBy: persisted.sortBy };
    searchInput.value = persisted.query;
    sortSelect.value = persisted.sortBy;
}

loadAndRender(state.query);
sortSelect.addEventListener("change", () => {
    state = { ...state, sortBy: sortSelect.value as SortField };
    savePrefs({ query: searchInput.value.trim(), sortBy: state.sortBy });
    loadAndRender(state.query);
});

function debounce<Args extends unknown[]>(
    fn: (...args: Args) => void,
    delay: number
): (...args: Args) => void {
    let timer: ReturnType<typeof setTimeout> | undefined;
    return (...args: Args) => {
        if (timer !== undefined) {
            clearTimeout(timer);
        }
        timer = setTimeout(() => fn(...args), delay);
    };
}

const debouncedLoad = debounce((query: string) => {
    state.query = query ?? "";
    loadAndRender(query);
}, 300);

searchInput.addEventListener("input", () => {
    debouncedLoad(searchInput.value.trim());
    savePrefs({ query: searchInput.value.trim(), sortBy: state.sortBy });
});

document.addEventListener("keydown", (event) => {
    const activeEl = document.activeElement;
    if (activeEl instanceof HTMLInputElement 
                || activeEl instanceof HTMLTextAreaElement) {
        return;
    }
    let should_render = true;
    if (event.key === "ArrowDown") {
        state.focusedIndex += 1;
        state.focusedIndex = Math.min(state.focusedIndex, state.posts.length - 1);
        event.preventDefault();
    } else if (event.key === "ArrowUp") {
        state.focusedIndex -= 1;
        state.focusedIndex = Math.max(state.focusedIndex, -1);
        event.preventDefault();
    } else if (event.key === "Enter" && state.focusedIndex >= 0) {
        let art = postList.children[state.focusedIndex] as HTMLElement;
        const toggle = art.querySelector<HTMLButtonElement>("button.toggle-btn");
        if (toggle) {
            toggle.click();
        }
        should_render = false;
        event.preventDefault();
    } else if (event.key === "Escape") {
        state.focusedIndex = -1;
    }

    if (should_render) {
        render();
        if (state.focusedIndex >= 0) {
            let article = postList.children[state.focusedIndex] as HTMLElement;
            article.scrollIntoView({ block: "nearest" });
        }
    }
});

// withRetry test.
// withRetry(() => api.getPosts("kanapak"), 3, 1000).then(result => {
//     console.log("Finish");
// });

document.addEventListener("visibilitychange", () => {
    state.visible = (document.visibilityState === "visible");
});

function startPolling(container: HTMLElement, info: HTMLElement, post: Post): void {
    if (state.activePolls.has(post.id)) {
        return;
    }
    let ago = 0;
    info.textContent = `Updated ${ago} seconds ago`;
    const interv = setInterval(() => {
        if (!state.visible) return;
        ago = (ago + 1) % 10;
        if (ago === 0) {
            renderComments(post, container);
        }
        info.textContent = `Updated ${ago} seconds ago`;

    }, 1000);


    state.activePolls.set(post.id, interv);
}

function stopPolling(postId: number): void {
    const interval = state.activePolls.get(postId);
    if (interval !== undefined) {
        clearInterval(interval);
        state.activePolls.delete(postId);
    }
}