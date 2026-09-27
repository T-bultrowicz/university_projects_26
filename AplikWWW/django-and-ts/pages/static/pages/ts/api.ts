type Result<T> =
    | { ok: true;  data: T }
    | { ok: false; error: string; status?: number };

interface Post {
    id:       number;
    title:    string;
    slug:     string;
    body:     string;
    pub_date: string;    // ISO string from Django
    category: string | null;
}

interface Comment {
    id: number;
    author: string;
    body: string;
    created: string;
}

type CreateCommentPayload = Pick<Comment, "author" | "body">;
type UpdatePostPayload = Partial<Pick<Post, "title" | "body" | "slug">>;

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

class BlogApiClient {
    async getPosts(search?: string): Promise<Result<{ posts: Post[] }>> {
        const qs = search ? `?search=${encodeURIComponent(search)}` : "";
        return apiFetch("GET", `/api/posts/${qs}`);
    }

    async getPost(id: number): Promise<Result<Post>> {
        return apiFetch("GET", `/api/posts/${id}/`);
    }

    async getComments(postId: number): Promise<Result<{comments: Comment[]}>> {
        return apiFetch("GET", `/api/posts/${postId}/comments/`);
    }

    async createComment(
        postId: number,
        payload: CreateCommentPayload
    ): Promise<Result<Comment>> {
        return apiFetch("POST", `/api/posts/${postId}/comments/`, payload);
    }

    async deleteComment(postId: number, commentId: number): Promise<Result<null>> {
        return apiFetch("DELETE", `/api/posts/${postId}/comments/${commentId}/`);
    }

    async updatePost(
        id: number,
        payload: UpdatePostPayload
    ): Promise<Result<Post>> {
        return apiFetch("PATCH", `/api/posts/${id}/`, payload);
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

function validateComment(author: string, body: string): ValidationResult {
    author.trim(); body.trim();
    let errors: Record<string, string> = {};
    if (author.length < 2) {
        errors.auth = "Must be at least 2 characters long";
    } else if (author.length > 50) {
        errors.auth = "Must be at most 50 characters long";
    }
    if (body.length < 10) {
        errors.body = "Must be at least 10 characters long";
    } else if (body.length > 500) {
        errors.body = "Must be at most 500 characters long";
    }
    if (Object.keys(errors).length > 0) {
        return { valid: false, errors };
    }
    return { valid: true };
}

const api = new BlogApiClient();

export { api, BlogApiClient, Post, Comment, Result, CreateCommentPayload, UpdatePostPayload, withRetry, validateComment };