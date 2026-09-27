"use strict";
(() => {
  // pages/static/pages/ts/api.ts
  function getCsrfToken() {
    const match = document.cookie.match(/csrftoken=([^;]+)/);
    return match ? match[1] : "";
  }
  async function apiFetch(method, path, body) {
    try {
      const options = {
        method,
        headers: {
          "Content-Type": "application/json",
          "X-CSRFToken": getCsrfToken()
        }
      };
      if (body !== void 0) {
        options.body = JSON.stringify(body);
      }
      const res = await fetch(path, options);
      if (!res.ok) {
        let message = `HTTP ${res.status}`;
        try {
          const err = await res.json();
          message = err.error ?? message;
        } catch {
        }
        return { ok: false, error: message, status: res.status };
      }
      if (res.status === 204) {
        return { ok: true, data: {} };
      }
      const data = await res.json();
      return { ok: true, data };
    } catch (err) {
      return { ok: false, error: String(err) };
    }
  }
  var BlogApiClient = class {
    async getPosts(search) {
      const qs = search ? `?search=${encodeURIComponent(search)}` : "";
      return apiFetch("GET", `/api/posts/${qs}`);
    }
    async getPost(id) {
      return apiFetch("GET", `/api/posts/${id}/`);
    }
    async getComments(postId) {
      return apiFetch("GET", `/api/posts/${postId}/comments/`);
    }
    async createComment(postId, payload) {
      return apiFetch("POST", `/api/posts/${postId}/comments/`, payload);
    }
    async deleteComment(postId, commentId) {
      return apiFetch("DELETE", `/api/posts/${postId}/comments/${commentId}/`);
    }
    async updatePost(id, payload) {
      return apiFetch("PATCH", `/api/posts/${id}/`, payload);
    }
  };
  function validateComment(author, body) {
    author.trim();
    body.trim();
    let errors = {};
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
  var api = new BlogApiClient();

  // pages/static/pages/ts/main.ts
  function getElement(id) {
    const el = document.getElementById(id);
    if (el === null) {
      throw new Error(`Element #${id} not found`);
    }
    return el;
  }
  var searchInput = getElement("search-input");
  var postList = getElement("post-list");
  var statusBar = getElement("status-bar");
  var sortSelect = getElement("sort-select");
  var state = {
    posts: [],
    query: "",
    status: "idle",
    error: "",
    sortBy: "date",
    focusedIndex: -1,
    activePolls: /* @__PURE__ */ new Map(),
    visible: true
  };
  async function loadAndRender(query) {
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
  function highlightMatch(query, title) {
    let res = document.createElement("span");
    if (query === "") {
      res.innerHTML = title;
      return res;
    }
    query = query.replace(/[-\/\\^$*+?.()|[\]{}<>]/g, "\\$&");
    title = title.replace(new RegExp(`${query}`, "gi"), `<mark>${query}</mark>`);
    res.innerHTML = title;
    return res;
  }
  function savePrefs(prefs) {
    localStorage.setItem("blog_prefs", JSON.stringify(prefs));
  }
  function isPersistedState(data) {
    return typeof data === "object" && typeof data.query === "string" && (data.sortBy === "title" || data.sortBy === "date" || data.sortBy === "category");
  }
  function loadPrefs() {
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
  function sortPosts(posts, by) {
    const sorted = [...posts];
    switch (by) {
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
  function renderPost(post) {
    const article = document.createElement("article");
    const heading = document.createElement("h2");
    heading.appendChild(highlightMatch(state.query, post.title));
    const excerpt = document.createElement("p");
    excerpt.textContent = post.body.slice(0, 120) + "\u2026";
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
      excerpt.hidden = !isExpanded;
      if (!isExpanded) {
        renderComments(post, comments);
        startPolling(comments, lastUpdated, post);
      } else {
        stopPolling(post.id);
      }
    });
    article.addEventListener("keydown", (event) => {
      if (event.key === "Enter") {
        toggle.click();
      }
    });
    article.tabIndex = 0;
    article.append(heading, excerpt, fullBody, comments, lastUpdated, toggle);
    renderCommentForm(post.id, article);
    return article;
  }
  async function renderComments(post, commentContainer) {
    commentContainer.innerHTML = "<p>Loading...</p>";
    const comms = await api.getComments(post.id);
    commentContainer.innerHTML = "";
    if (comms.ok) {
      for (const comment of comms.data.comments) {
        let div = document.createElement("div");
        div.innerHTML = `<strong>${comment.author}</strong>: ${comment.body}`;
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
  function renderCommentForm(postId, container) {
    const form = document.createElement("form");
    container.appendChild(form);
    const authorInput = document.createElement("input");
    authorInput.type = "text";
    authorInput.placeholder = "Your name";
    authorInput.required = true;
    const bodyTextarea = document.createElement("textarea");
    bodyTextarea.placeholder = "Your comment";
    bodyTextarea.required = true;
    bodyTextarea.rows = 3;
    const submitBtn = document.createElement("button");
    submitBtn.type = "submit";
    submitBtn.textContent = "Post Comment";
    const feedback = document.createElement("p");
    form.append(authorInput, bodyTextarea, submitBtn, feedback);
    form.addEventListener("submit", async (event) => {
      event.preventDefault();
      submitBtn.disabled = true;
      feedback.textContent = "";
      form.querySelectorAll(".error").forEach((el) => el.remove());
      const validation = validateComment(authorInput.value, bodyTextarea.value);
      if (!validation.valid) {
        feedback.textContent = "Please fix the errors in the form.";
        Object.entries(validation.errors).forEach(([key, value]) => {
          let obj = document.createElement("span");
          obj.classList.add("error");
          obj.textContent = value;
          if (key === "auth") {
            authorInput.insertAdjacentElement("afterend", obj);
          } else if (key === "body") {
            bodyTextarea.insertAdjacentElement("afterend", obj);
          }
        });
        submitBtn.disabled = false;
        return;
      }
      feedback.textContent = "Posting\u2026";
      const result = await api.createComment(postId, {
        author: authorInput.value.trim(),
        body: bodyTextarea.value.trim()
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
  function render() {
    postList.innerHTML = "";
    if (state.status === "loading") {
      statusBar.textContent = "Loading\u2026";
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
  var persisted = loadPrefs();
  if (persisted !== null) {
    state = { ...state, query: persisted.query, sortBy: persisted.sortBy };
    searchInput.value = persisted.query;
    sortSelect.value = persisted.sortBy;
  }
  loadAndRender(state.query);
  sortSelect.addEventListener("change", () => {
    state = { ...state, sortBy: sortSelect.value };
    savePrefs({ query: searchInput.value.trim(), sortBy: state.sortBy });
    loadAndRender(state.query);
  });
  function debounce(fn, delay) {
    let timer;
    return (...args) => {
      if (timer !== void 0) {
        clearTimeout(timer);
      }
      timer = setTimeout(() => fn(...args), delay);
    };
  }
  var debouncedLoad = debounce((query) => {
    state.query = query ?? "";
    loadAndRender(query);
  }, 300);
  searchInput.addEventListener("input", () => {
    debouncedLoad(searchInput.value.trim());
    savePrefs({ query: searchInput.value.trim(), sortBy: state.sortBy });
  });
  document.addEventListener("keydown", (event) => {
    const activeEl = document.activeElement;
    if (activeEl instanceof HTMLInputElement || activeEl instanceof HTMLTextAreaElement) {
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
      let art = postList.children[state.focusedIndex];
      const toggle = art.querySelector("button.toggle-btn");
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
        let article = postList.children[state.focusedIndex];
        article.scrollIntoView({ block: "nearest" });
      }
    }
  });
  document.addEventListener("visibilitychange", () => {
    state.visible = document.visibilityState === "visible";
  });
  function startPolling(container, info, post) {
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
    }, 1e3);
    state.activePolls.set(post.id, interv);
  }
  function stopPolling(postId) {
    const interval = state.activePolls.get(postId);
    if (interval !== void 0) {
      clearInterval(interval);
      state.activePolls.delete(postId);
    }
  }
})();
//# sourceMappingURL=main.js.map
