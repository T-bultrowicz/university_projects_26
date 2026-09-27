"use strict";
(() => {
  // game/static/game/ts/api.ts
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
  var GameApiClient = class {
    // TODO: implement searching mechanism for mazes and runs, 
    // by modyfing getmazes, getruns and doing it in django backend.
    async getMazes(query = "") {
      const url = query.trim() !== "" ? `/api/mazes/?q=${encodeURIComponent(query.trim())}` : `/api/mazes/`;
      return apiFetch("GET", url);
    }
    async getMaze(id) {
      return apiFetch("GET", `/api/mazes/${id}/`);
    }
    async getRuns(mazeId) {
      const url = mazeId ? `/api/mazes/${mazeId}/runs/` : "/api/runs/";
      return apiFetch("GET", url);
    }
    async getRun(id) {
      return apiFetch("GET", `/api/runs/${id}/`);
    }
    async submitRun(mazeId, time) {
      return apiFetch("POST", `/api/mazes/${mazeId}/runs/`, { time });
    }
  };
  var api = new GameApiClient();

  // game/static/game/ts/play_browser.ts
  var searchInput = document.getElementById("search-input");
  var statusBar = document.getElementById("status-bar");
  var postList = document.getElementById("post-list");
  var debounceTimer;
  function escapeRegExp(string) {
    return string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  }
  function escapeHTML(str) {
    return str.replace(/[&<>'"]/g, (tag) => ({
      "&": "&amp;",
      "<": "&lt;",
      ">": "&gt;",
      "'": "&#39;",
      '"': "&quot;"
    })[tag] || tag);
  }
  function highlightText(text, query) {
    const safeText = escapeHTML(text);
    if (!query) return safeText;
    const safeQuery = escapeHTML(query);
    const escapedQuery = escapeRegExp(safeQuery);
    const regex = new RegExp(`(${escapedQuery})`, "gi");
    return safeText.replace(regex, "<mark>$1</mark>");
  }
  function renderMazes(mazes, query) {
    if (!postList) return;
    postList.innerHTML = "";
    if (mazes.length === 0) {
      postList.innerHTML = "<p>No mazes found matching your criteria.</p>";
      return;
    }
    mazes.forEach((maze) => {
      const card = document.createElement("div");
      card.className = "maze-card";
      const highlightedTitle = highlightText(maze.name, query);
      card.innerHTML = `
            <h3>${highlightedTitle}</h3>
            <a href="/play/${maze.name}/" class="play-btn">Play Now</a>
        `;
      postList.appendChild(card);
    });
  }
  async function fetchAndDisplay(query = "") {
    if (!statusBar || !postList) return;
    statusBar.textContent = "Searching...";
    const response = await api.getMazes(query);
    if (!response.ok) {
      statusBar.textContent = `Error: ${response.error}`;
      postList.innerHTML = "";
      return;
    }
    const mazesList = response.data.mazes || [];
    statusBar.textContent = `Found ${mazesList.length} levels.`;
    renderMazes(mazesList, query);
  }
  function init() {
    if (!searchInput) {
      console.error("Search input not found!");
      return;
    }
    searchInput.addEventListener("input", (e) => {
      const target = e.target;
      const query = target.value.trim();
      clearTimeout(debounceTimer);
      debounceTimer = setTimeout(() => {
        fetchAndDisplay(query);
      }, 300);
    });
    fetchAndDisplay();
  }
  document.addEventListener("DOMContentLoaded", init);
})();
//# sourceMappingURL=play_browser.js.map
