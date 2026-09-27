function identity<T>(value: T): T {
    return value;
}

function first<T>(arr: T[]): T | undefined {
    return arr[0];
}

function groupBy<T>(items: T[], keyFn: (item: T) => string): Record<string, T[]> {
    let groups: Record<string, T[]> = {};
    for (const item of items) {
        const key = keyFn(item);
        if (!groups[key]) {
            groups[key] = [];
        }
        groups[key].push(item);
    }
    return groups;
}

function pipe<T>(...fns: Array<(arg: T) => T>): (arg: T) => T {
    return (arg : T) => fns.reduceRight((acc, fn) => fn(acc), arg);
}

function memoize<A extends PropertyKey, R>(fn: (arg : A) => Promise<R>): (arg: A) => Promise<R> {
    let cache: Record<A, Promise<R>> = {} as Record<A, Promise<R>>;
    return (arg: A) => {
        if (!cache[arg]) {
            cache[arg] = fn(arg);
        }
        return cache[arg];
    };
}

export { identity, first, groupBy, pipe, memoize };