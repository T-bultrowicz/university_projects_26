interface Circle    { kind: "circle";    radius: number; }
interface Rectangle { kind: "rectangle"; width: number; height: number; }
interface Triangle  { kind: "triangle";  base: number; height: number; }
type Shape = Circle | Rectangle | Triangle;

function area(shape: Shape): number {
    switch (shape.kind) {
        case "circle":
            return Math.PI * shape.radius ** 2;
        case "rectangle":
            return shape.width * shape.height;
        case "triangle":
            return 0.5 * shape.base * shape.height;
    }
}

function svgEl(
    tag: string,
    attrs: Record<string, string | number>
): SVGElement {
    const el = document.createElementNS("http://www.w3.org/2000/svg", tag);
    for (const [k, v] of Object.entries(attrs)) {
        el.setAttribute(k, String(v));
    }
    return el as SVGElement;
}

function renderShape(shape: Shape): SVGElement {
    const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
    svg.setAttribute("width", "120");
    svg.setAttribute("height", "120");
    svg.setAttribute("viewBox", "0 0 120 120");

    switch (shape.kind) {
        case "circle":
            svg.appendChild(svgEl("circle", { cx: 60, cy: 60, r: shape.radius, fill: "steelblue" }));
            break;
        case "rectangle":
            svg.appendChild(svgEl("rect", { width : shape.width, height: shape.height, fill : "coral" }));
            break;
        case "triangle":
            const h = shape.height;
            const b = shape.base;
            const points = `${60 - b/2},${60 + h/2} ${60 + b/2},${60 + h/2} 60,${60 - h/2}`;
            svg.appendChild(svgEl("polygon", { points, fill: "seagreen" }));
            break;
    }
    return svg;
}

export { Circle, Rectangle, Triangle, Shape, area, renderShape };