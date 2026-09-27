interface Category {
    id:   number;
    name: string;
    slug: string;
}

interface Post {
    id:       number;
    title:    string;
    slug:     string;
    body:     string;
    pubDate:  string;
    category: Category | null;   // a field that is either a Category or nothing — union types are introduced in Phase 4
}

interface Comment {
    id:      number;
    author:  string;
    body:    string;
    created: string;
}

function summarise(post: Post): string {
    return `${post.title} (${post.category ? post.category.name : "Uncategorised"}) — ${post.body.substring(0, 50)}...`;
};

function filterByCategory(posts: Post[], categorySlug: string): Post[] {
    return posts.filter(post => post.category?.slug === categorySlug);
}

function sortPosts(posts: Post[], by: "title" | "date" | "category"): Post[] {
    posts.sort((a, b) => {
        switch (by) {
            case "title":
                return a.title.localeCompare(b.title);
            case "date":
                return new Date(a.pubDate).getTime() - new Date(b.pubDate).getTime();
            case "category":
                if (a.category === null)
                    return 1;
                if (b.category === null)
                    return -1;
                return a.category.name.localeCompare(b.category.name);
        }
    });
    return posts;
}

function validatePost(data: unknown): { ok: true; post: Post } | { ok: false; reason: string } {
    if (typeof data !== "object" || data === null) {
        return { ok: false, reason: "Not an object" };
    }
    let validated : Post = { id: 0, title: "", slug: "", body: "", pubDate: "", category: null };

    if (!("id" in data) || typeof data.id !== "number") {
        return { ok: false, reason: "ID is not valid / lacking!" };
    }
    validated.id = data.id as number;

    if (!("title" in data) || typeof data.title !== "string") {
        return { ok: false, reason: "Title is not valid / lacking!" };
    }
    validated.title = data.title as string;

    if (!("body" in data) || typeof data.body !== "string") {
        return { ok: false, reason: "Body is not valid / lacking!" };
    }
    validated.body = data.body as string;

    if (!("slug" in data) || typeof data.slug !== "string") {
        return { ok: false, reason: "Slug is not valid / lacking!" };
    }
    validated.slug = data.slug as string;

    if (!("pubDate" in data) || typeof data.pubDate !== "string") {
        return { ok: false, reason: "Publication date is not valid / lacking!" };
    }
    validated.pubDate = data.pubDate as string;

    if (!("category" in data)) {
        return { ok: false, reason: "Category is not valid / lacking!" };
    } else if (data.category !== null) {
        if (typeof data.category !== "object" || data.category === null ||
            !("id" in data.category) || typeof data.category.id !== "number" ||
            !("name" in data.category) || typeof data.category.name !== "string" ||
            !("slug" in data.category) || typeof data.category.slug !== "string") {
            return { ok: false, reason: "Category is not valid / lacking!" };
        }
        validated.category = {
            id: data.category.id,
            name: data.category.name,
            slug: data.category.slug,
        };
    }

    return { ok: true, post: validated };
}


export { Post, Comment, Category, summarise, filterByCategory, sortPosts, validatePost };