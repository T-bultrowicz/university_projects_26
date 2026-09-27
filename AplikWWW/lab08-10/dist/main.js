"use strict";
(() => {
  // src/async.ts
  function delay(ms) {
    return new Promise((resolve) => setTimeout(resolve, ms));
  }
  async function countdown(element) {
    for (let i = 5; i >= 0; i--) {
      element.textContent = `${i}\u2026`;
      await delay(1e3);
    }
    element.textContent = "Go!";
  }
  async function fetchTodo(id) {
    const res = await fetch(`https://jsonplaceholder.typicode.com/todos/${id}`);
    if (!res.ok) {
      throw new Error(`HTTP ${res.status}`);
    }
    const data = await res.json();
    return data;
  }
  async function fetchWithTimeout(url2, ms) {
    const timeout = new Promise((_, reject) => delay(ms).then(() => reject(new Error("timeout"))));
    const fetchPromise = fetch(url2).then(async (res) => {
      if (!res.ok) {
        throw new Error(`HTTP ${res.status}`);
      }
      const data = res.json();
      return data;
    });
    return Promise.race([timeout, fetchPromise]);
  }

  // src/blog.ts
  function summarise(post) {
    return `${post.title} (${post.category ? post.category.name : "Uncategorised"}) \u2014 ${post.body.substring(0, 50)}...`;
  }
  function filterByCategory(posts2, categorySlug) {
    return posts2.filter((post) => post.category?.slug === categorySlug);
  }
  function sortPosts(posts2, by) {
    posts2.sort((a, b) => {
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
    return posts2;
  }
  function validatePost(data) {
    if (typeof data !== "object" || data === null) {
      return { ok: false, reason: "Not an object" };
    }
    let validated = { id: 0, title: "", slug: "", body: "", pubDate: "", category: null };
    if (!("id" in data) || typeof data.id !== "number") {
      return { ok: false, reason: "ID is not valid / lacking!" };
    }
    validated.id = data.id;
    if (!("title" in data) || typeof data.title !== "string") {
      return { ok: false, reason: "Title is not valid / lacking!" };
    }
    validated.title = data.title;
    if (!("body" in data) || typeof data.body !== "string") {
      return { ok: false, reason: "Body is not valid / lacking!" };
    }
    validated.body = data.body;
    if (!("slug" in data) || typeof data.slug !== "string") {
      return { ok: false, reason: "Slug is not valid / lacking!" };
    }
    validated.slug = data.slug;
    if (!("pubDate" in data) || typeof data.pubDate !== "string") {
      return { ok: false, reason: "Publication date is not valid / lacking!" };
    }
    validated.pubDate = data.pubDate;
    if (!("category" in data)) {
      return { ok: false, reason: "Category is not valid / lacking!" };
    } else if (data.category !== null) {
      if (typeof data.category !== "object" || data.category === null || !("id" in data.category) || typeof data.category.id !== "number" || !("name" in data.category) || typeof data.category.name !== "string" || !("slug" in data.category) || typeof data.category.slug !== "string") {
        return { ok: false, reason: "Category is not valid / lacking!" };
      }
      validated.category = {
        id: data.category.id,
        name: data.category.name,
        slug: data.category.slug
      };
    }
    return { ok: true, post: validated };
  }

  // src/shapes.ts
  function area(shape) {
    switch (shape.kind) {
      case "circle":
        return Math.PI * shape.radius ** 2;
      case "rectangle":
        return shape.width * shape.height;
      case "triangle":
        return 0.5 * shape.base * shape.height;
    }
  }
  function svgEl(tag, attrs) {
    const el = document.createElementNS("http://www.w3.org/2000/svg", tag);
    for (const [k, v] of Object.entries(attrs)) {
      el.setAttribute(k, String(v));
    }
    return el;
  }
  function renderShape(shape) {
    const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    svg.setAttribute("width", "120");
    svg.setAttribute("height", "120");
    svg.setAttribute("viewBox", "0 0 120 120");
    switch (shape.kind) {
      case "circle":
        svg.appendChild(svgEl("circle", { cx: 60, cy: 60, r: shape.radius, fill: "steelblue" }));
        break;
      case "rectangle":
        svg.appendChild(svgEl("rect", { width: shape.width, height: shape.height, fill: "coral" }));
        break;
      case "triangle":
        const h = shape.height;
        const b = shape.base;
        const points = `${60 - b / 2},${60 + h / 2} ${60 + b / 2},${60 + h / 2} 60,${60 - h / 2}`;
        svg.appendChild(svgEl("polygon", { points, fill: "seagreen" }));
        break;
    }
    return svg;
  }

  // src/utils.ts
  function groupBy(items, keyFn) {
    let groups = {};
    for (const item of items) {
      const key = keyFn(item);
      if (!groups[key]) {
        groups[key] = [];
      }
      groups[key].push(item);
    }
    return groups;
  }
  function pipe(...fns) {
    return (arg) => fns.reduceRight((acc, fn) => fn(acc), arg);
  }
  function memoize(fn) {
    let cache = {};
    return (arg) => {
      if (!cache[arg]) {
        cache[arg] = fn(arg);
      }
      return cache[arg];
    };
  }

  // src/main.ts
  function show(label, value) {
    const output2 = document.getElementById("output");
    const line = document.createElement("p");
    line.textContent = `${label}: ${String(value)}`;
    output2.appendChild(line);
  }
  function greet(name, times) {
    return `Hello, ${name}! `.repeat(times).trim();
  }
  function clamp(value, min, max) {
    if (value < min)
      return min;
    else if (value > max)
      return max;
    return value;
  }
  function formatDuration(totalSeconds) {
    if (totalSeconds < 60)
      return `${totalSeconds}s`;
    else if (totalSeconds < 3600)
      return `${Math.trunc(totalSeconds / 60)}m ${totalSeconds % 60}s`;
    return `${Math.trunc(totalSeconds / 3600)}h ${Math.trunc(totalSeconds / 60) % 60}m ${totalSeconds % 60}s`;
  }
  var testCases = [
    [0, "0s"],
    [5, "5s"],
    [62, "1m 2s"],
    [3661, "1h 1m 1s"],
    [86400, "24h 0m 0s"]
  ];
  var table = document.createElement("table");
  table.innerHTML = "<tr><th>Input</th><th>Expected</th><th>Got</th><th>\u2713</th></tr>";
  for (const [input, expected] of testCases) {
    const got = formatDuration(input);
    const pass = got === expected;
    const row = document.createElement("tr");
    row.innerHTML = `<td>${input}</td><td>${expected}</td><td>${got}</td>
                     <td>${pass ? "\u2705" : "\u274C"}</td>`;
    if (!pass) row.classList.add("error");
    table.appendChild(row);
  }
  document.getElementById("output").appendChild(table);
  show("greet", greet("World", 2));
  show("clamp(15, 0, 10)", clamp(15, 0, 10));
  show("clamp(-5, 0, 10)", clamp(-5, 0, 10));
  show("clamp(-5, -11, 10)", clamp(-5, -11, 10));
  var counter = document.createElement("h2");
  document.getElementById("output").appendChild(counter);
  countdown(counter);
  async function renderTodos() {
    const container = document.createElement("div");
    container.innerHTML = "<h3>Todos from API</h3>";
    document.getElementById("output").appendChild(container);
    for (const id of [1, 2, 3]) {
      const todo = await fetchTodo(id);
      const p = document.createElement("p");
      p.textContent = `${todo.completed ? "\u2705" : "\u2B1C"} ${todo.title}`;
      container.appendChild(p);
    }
  }
  renderTodos();
  function showTestResult(promise) {
    const container = document.createElement("div");
    const p = document.createElement("p");
    promise.then(
      () => {
        p.textContent = `\u2705 Succeeded`;
      },
      () => {
        p.textContent = `\u274C timed out`;
      }
    );
    container.appendChild(p);
    document.getElementById("output").appendChild(container);
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
        p.textContent = `${val[i].completed ? "\u2705" : "\u2B1C"} ${val[i].title}`;
        container.appendChild(p);
      }
    }, () => {
      throw new Error("Not worked");
    });
    document.getElementById("output").appendChild(container);
    let elapsed = performance.now() - start;
    console.log(elapsed);
  }
  function fetchSequence() {
    const container = document.createElement("div");
    let start = performance.now();
    for (let i = 1; i < 11; i++) {
      fetchTodo(i).then((val) => {
        const p = document.createElement("p");
        p.textContent = `${val.completed ? "\u2705" : "\u2B1C"} ${val.title}`;
        container.appendChild(p);
      }, () => {
        throw new Error("Not worked");
      });
    }
    document.getElementById("output").appendChild(container);
    let elapsed = performance.now() - start;
    console.log(elapsed);
  }
  var url = "https://jsonplaceholder.typicode.com/todos/1";
  showTestResult(fetchWithTimeout(url, 1));
  showTestResult(fetchWithTimeout(url, 5e3));
  fetchParallel();
  fetchSequence();
  var posts = [
    {
      id: 1,
      title: "Hello TypeScript",
      slug: "hello-ts",
      body: "TypeScript is JavaScript with types. It compiles to plain JS.",
      pubDate: "2025-01-01",
      category: { id: 1, name: "Tech", slug: "tech" }
    },
    {
      id: 2,
      title: "CSS Grid",
      slug: "css-grid",
      body: "CSS Grid is a two-dimensional layout system for the web.",
      pubDate: "2025-01-15",
      category: { id: 2, name: "Frontend", slug: "frontend" }
    },
    {
      id: 3,
      title: "Django REST",
      slug: "django-rest",
      body: "Build a REST API with Django and serve JSON to any client.",
      pubDate: "2025-02-01",
      category: { id: 1, name: "Tech", slug: "tech" }
    }
  ];
  function renderPostCard(post) {
    const card = document.createElement("article");
    card.innerHTML = `<h3>${post.title}</h3><p>${summarise(post)}</p>`;
    return card;
  }
  var output = document.getElementById("output");
  var select = document.createElement("select");
  var sortedPosts = posts;
  select.innerHTML = `
    <option value="title">Title</option>
    <option value="date">Date</option>
    <option value="category">Category</option>
`;
  select.addEventListener("change", () => {
    sortedPosts = sortPosts(posts, select.value);
    postsSection.innerHTML = `<h2>All posts sorted by ${select.value} </h2>`;
    for (const post of sortedPosts) {
      postsSection.appendChild(renderPostCard(post));
    }
  });
  output.appendChild(select);
  var postsSection = document.createElement("div");
  postsSection.innerHTML = "<h2>All posts</h2>";
  output.appendChild(postsSection);
  for (const post of sortedPosts) {
    postsSection.appendChild(renderPostCard(post));
  }
  var techPosts = filterByCategory(posts, "tech");
  var filteredSection = document.createElement("div");
  filteredSection.innerHTML = `<h3>Tech posts: ${techPosts.map((p) => p.title).join(", ")}</h3>`;
  output.appendChild(filteredSection);
  var shapes = [
    { kind: "circle", radius: 20 },
    { kind: "rectangle", width: 80, height: 80 },
    { kind: "triangle", base: 10, height: 60 }
  ];
  var shapesSection = document.createElement("div");
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
  var validPostSection = document.createElement("div");
  var textarea = document.createElement("textarea");
  textarea.placeholder = "Enter post data as JSON";
  var validateButton = document.createElement("button");
  validateButton.textContent = "Validate post";
  var validationResult = document.createElement("p");
  validPostSection.appendChild(textarea);
  validPostSection.appendChild(validateButton);
  validPostSection.appendChild(validationResult);
  validateButton.addEventListener("click", () => {
    validationResult.textContent = "";
    const data = JSON.parse(textarea.value);
    const result = validatePost(data);
    if (result.ok) {
      validationResult.textContent = `\u2705 Valid post: ${result.post.title}`;
    } else {
      validationResult.textContent = `\u274C Invalid post: ${result.reason}`;
    }
  });
  output.appendChild(validPostSection);
  var groupedSection = document.createElement("div");
  var groupedPosts = groupBy(posts, (post) => post.category?.slug ?? "uncategorised");
  for (const [key, value] of Object.entries(groupedPosts)) {
    const categorySection = document.createElement("div");
    categorySection.innerHTML = `<h3>${key}:</h3>`;
    for (const post of value) {
      categorySection.appendChild(renderPostCard(post));
    }
    groupedSection.appendChild(categorySection);
  }
  output.appendChild(groupedSection);
  var process = pipe(
    (posts2) => posts2.filter((p) => p.category !== null),
    (posts2) => posts2.filter((p) => p.pubDate >= "2025-01-10"),
    (posts2) => posts2.sort((a, b) => a.title.localeCompare(b.title)),
    (posts2) => posts2.slice(0, 5)
  );
  var pipeSection = document.createElement("div");
  pipeSection.innerHTML = "<h2>Processed posts</h2>";
  for (const post of process(posts)) {
    pipeSection.appendChild(renderPostCard(post));
  }
  output.appendChild(pipeSection);
  var memoizeSection = document.createElement("div");
  memoizeSection.innerHTML = "<h2>Memoized fetch</h2>";
  var memoizedFetch = memoize(fetchTodo);
  var testFetch = async (id) => {
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
})();
//# sourceMappingURL=main.js.map
