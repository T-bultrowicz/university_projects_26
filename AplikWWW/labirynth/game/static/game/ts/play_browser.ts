import { api, Maze } from './api';

const searchInput = document.getElementById('search-input') as HTMLInputElement | null;
const statusBar = document.getElementById('status-bar') as HTMLParagraphElement | null;
const postList = document.getElementById('post-list') as HTMLDivElement | null;
let debounceTimer: number;

function escapeRegExp(string: string): string {
    return string.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function escapeHTML(str: string): string {
    return str.replace(/[&<>'"]/g, tag => ({
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        "'": '&#39;',
        '"': '&quot;'
    }[tag] || tag));
}

function highlightText(text: string, query: string): string {
    const safeText = escapeHTML(text);
    if (!query) return safeText;
    const safeQuery = escapeHTML(query);
    const escapedQuery = escapeRegExp(safeQuery);

    const regex = new RegExp(`(${escapedQuery})`, 'gi');
    
    return safeText.replace(regex, '<mark>$1</mark>');
}

// Zmiana: dodano parametr `query`, abyśmy wiedzieli co podświetlić
function renderMazes(mazes: Maze[], query: string): void {
    if (!postList) return;

    postList.innerHTML = ''; 

    if (mazes.length === 0) {
        postList.innerHTML = '<p>No mazes found matching your criteria.</p>';
        return;
    }

    mazes.forEach(maze => {
        const card = document.createElement('div');
        card.className = 'maze-card'; 

        const highlightedTitle = highlightText(maze.name, query);
        
        card.innerHTML = `
            <h3>${highlightedTitle}</h3>
            <a href="/play/${maze.name}/" class="play-btn">Play Now</a>
        `;
        
        postList.appendChild(card);
    });
}

/**
 * Główna funkcja pobierająca dane i aktualizująca UI.
 */
async function fetchAndDisplay(query: string = ''): Promise<void> {
    if (!statusBar || !postList) return;

    statusBar.textContent = 'Searching...';

    const response = await api.getMazes(query);

    if (!response.ok) {
        statusBar.textContent = `Error: ${response.error}`;
        postList.innerHTML = '';
        return;
    }

    const mazesList = (response.data as any).mazes || []; 

    statusBar.textContent = `Found ${mazesList.length} levels.`;

    renderMazes(mazesList, query);
}

function init(): void {
    if (!searchInput) {
        console.error("Search input not found!");
        return;
    }

    searchInput.addEventListener('input', (e) => {
        const target = e.target as HTMLInputElement;
        const query = target.value.trim();

        clearTimeout(debounceTimer);

        // Zapytanie wysyła się wraz z tekstem po 300ms od zakończenia pisania
        debounceTimer = setTimeout(() => {
            fetchAndDisplay(query);
        }, 300);
    });

    fetchAndDisplay();
}

document.addEventListener('DOMContentLoaded', init);