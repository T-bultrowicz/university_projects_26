// src/status.ts
import { Post } from "./blog";


// An enum compiles to a real JavaScript object.
enum PostStatus {
    Draft     = "draft",
    Published = "published",
    Archived  = "archived",
}

// A union type: the value must be one of these strings.
type HttpMethod = "GET" | "POST" | "PUT" | "PATCH" | "DELETE";

interface ApiRequest {
    method:  HttpMethod;
    path:    string;
    body?:   unknown;    // ? makes the field optional
}

// A type guard narrows a wide type to a specific one.
function isPost(value: unknown): value is Post {
    return (
        typeof value === "object" &&
        value !== null &&
        "title" in value &&
        "slug"  in value
    );
}

// TODO: Write a function describeRequest(req: ApiRequest): string
// that returns e.g. "GET /api/posts/" or "POST /api/posts/ (has body)"
function describeRequest(req: ApiRequest): string {
    let res = `${req.method} ${req.path}`;
    if (req.body !== undefined) {
        res += " (has body)";
    }
    return res;
}

export { PostStatus, HttpMethod, ApiRequest, isPost, describeRequest };