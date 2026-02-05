/**
 * MovieFlix - Netflix-Style Movie Recommender
 * 
 * Features:
 * - Debounced autocomplete search
 * - Movie selection and recommendation display
 * - Horizontal carousel with navigation
 * - Smooth animations
 */

// ============================================
// Configuration
// ============================================
const API_BASE_URL = 'http://localhost:5000';
const DEBOUNCE_DELAY = 300;

// ============================================
// DOM Elements
// ============================================
const elements = {
    searchInput: document.getElementById('searchInput'),
    clearBtn: document.getElementById('clearBtn'),
    autocompleteDropdown: document.getElementById('autocompleteDropdown'),
    autocompleteList: document.getElementById('autocompleteList'),
    selectedMovieSection: document.getElementById('selectedMovieSection'),
    selectedMovie: document.getElementById('selectedMovie'),
    recommendationsSection: document.getElementById('recommendationsSection'),
    recommendationsCarousel: document.getElementById('recommendationsCarousel'),
    genreCarousel: document.getElementById('genreCarousel'),
    movieName: document.getElementById('movieName'),
    genreName: document.getElementById('genreName'),
    errorSection: document.getElementById('errorSection'),
    errorMessage: document.getElementById('errorMessage'),
    loadingOverlay: document.getElementById('loadingOverlay')
};

// ============================================
// State
// ============================================
let selectedMovie = null;
let allRecommendations = [];
let selectedIndex = -1;

// ============================================
// Utility Functions
// ============================================

/**
 * Debounce function to limit API calls
 */
function debounce(func, wait) {
    let timeout;
    return function executedFunction(...args) {
        const later = () => {
            clearTimeout(timeout);
            func(...args);
        };
        clearTimeout(timeout);
        timeout = setTimeout(later, wait);
    };
}

/**
 * Escape HTML to prevent XSS
 */
function escapeHtml(text) {
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
}

/**
 * Get movie emoji based on genre
 */
function getGenreEmoji(genre) {
    const emojis = {
        'Action': '🎬',
        'Comedy': '😂',
        'Drama': '🎭',
        'Horror': '👻',
        'Sci-Fi': '🚀',
        'Romance': '💕',
        'Thriller': '🔪',
        'Animation': '🎨'
    };
    return emojis[genre] || '🎥';
}

// ============================================
// API Functions
// ============================================

/**
 * Search movies by name
 */
async function searchMovies(query) {
    if (!query.trim()) return [];

    try {
        const response = await fetch(`${API_BASE_URL}/search?name=${encodeURIComponent(query)}`);
        if (!response.ok) throw new Error('Search failed');
        return await response.json();
    } catch (error) {
        console.error('Search error:', error);
        return [];
    }
}

/**
 * Get recommendations for a movie
 */
async function getRecommendations(movieId) {
    try {
        const response = await fetch(`${API_BASE_URL}/recommend?movie_id=${movieId}`);
        if (!response.ok) throw new Error('Failed to get recommendations');
        return await response.json();
    } catch (error) {
        console.error('Recommendation error:', error);
        throw error;
    }
}

// ============================================
// UI Functions
// ============================================

/**
 * Show/hide loading overlay
 */
function setLoading(isLoading) {
    elements.loadingOverlay.hidden = !isLoading;
}

/**
 * Show error message
 */
function showError(message) {
    elements.errorMessage.textContent = message;
    elements.errorSection.hidden = false;
    elements.recommendationsSection.hidden = true;
    elements.selectedMovieSection.hidden = true;
}

/**
 * Hide error message
 */
function hideError() {
    elements.errorSection.hidden = true;
}

/**
 * Show/hide clear button
 */
function updateClearButton() {
    elements.clearBtn.hidden = !elements.searchInput.value;
}

/**
 * Render autocomplete dropdown
 */
function renderAutocomplete(movies) {
    if (movies.length === 0) {
        elements.autocompleteDropdown.hidden = true;
        return;
    }

    elements.autocompleteList.innerHTML = movies.map((movie, index) => `
        <div class="autocomplete-item ${index === selectedIndex ? 'selected' : ''}" 
             data-id="${movie.id}" 
             data-index="${index}">
            <div class="autocomplete-poster">${getGenreEmoji(movie.genre)}</div>
            <div class="autocomplete-info">
                <div class="autocomplete-title">${escapeHtml(movie.title)}</div>
                <div class="autocomplete-meta">
                    <span class="genre-badge">${escapeHtml(movie.genre)}</span>
                    <span class="rating-badge">${movie.rating.toFixed(1)}</span>
                </div>
            </div>
        </div>
    `).join('');

    elements.autocompleteDropdown.hidden = false;

    // Add click handlers
    document.querySelectorAll('.autocomplete-item').forEach(item => {
        item.addEventListener('click', () => {
            const movieId = parseInt(item.dataset.id);
            const movie = movies.find(m => m.id === movieId);
            if (movie) selectMovie(movie);
        });
    });
}

/**
 * Hide autocomplete dropdown
 */
function hideAutocomplete() {
    elements.autocompleteDropdown.hidden = true;
    selectedIndex = -1;
}

/**
 * Render selected movie card
 */
function renderSelectedMovie(movie) {
    elements.selectedMovie.innerHTML = `
        <div class="selected-poster">${getGenreEmoji(movie.genre)}</div>
        <div class="selected-info">
            <h2 class="selected-title">${escapeHtml(movie.title)}</h2>
            <div class="selected-meta">
                <span class="selected-genre">${escapeHtml(movie.genre)}</span>
                <span class="selected-rating">${movie.rating.toFixed(1)}</span>
            </div>
        </div>
    `;
    elements.selectedMovieSection.hidden = false;
}

/**
 * Create movie card HTML
 */
function createMovieCard(movie) {
    return `
        <div class="movie-card" data-id="${movie.id}" onclick="handleCardClick(${movie.id})">
            <div class="card-poster">
                ${getGenreEmoji(movie.genre)}
                <span class="card-rating">${movie.rating.toFixed(1)}</span>
            </div>
            <div class="card-info">
                <h3 class="card-title">${escapeHtml(movie.title)}</h3>
                <p class="card-genre">${escapeHtml(movie.genre)}</p>
            </div>
        </div>
    `;
}

/**
 * Render recommendations carousel
 */
function renderRecommendations(recommendations, sourceMovie) {
    if (recommendations.length === 0) {
        showError('No recommendations found for this movie.');
        return;
    }

    hideError();
    allRecommendations = recommendations;

    // Update section titles
    elements.movieName.textContent = sourceMovie.title;
    elements.genreName.textContent = sourceMovie.genre;

    // Split recommendations
    const topRated = [...recommendations].sort((a, b) => b.rating - a.rating).slice(0, 15);
    const sameGenre = recommendations.filter(m => m.genre === sourceMovie.genre).slice(0, 15);

    // Render main carousel
    elements.recommendationsCarousel.innerHTML = topRated.map(createMovieCard).join('');

    // Render genre carousel
    if (sameGenre.length > 0) {
        elements.genreCarousel.innerHTML = sameGenre.map(createMovieCard).join('');
        document.getElementById('genreSection').hidden = false;
    } else {
        document.getElementById('genreSection').hidden = true;
    }

    elements.recommendationsSection.hidden = false;

    // Scroll to recommendations
    elements.selectedMovieSection.scrollIntoView({ behavior: 'smooth', block: 'start' });
}

// ============================================
// Event Handlers
// ============================================

/**
 * Handle search input change
 */
const handleSearchInput = debounce(async (event) => {
    const query = event.target.value.trim();
    updateClearButton();

    if (query.length < 2) {
        hideAutocomplete();
        return;
    }

    const movies = await searchMovies(query);
    renderAutocomplete(movies);
}, DEBOUNCE_DELAY);

/**
 * Handle movie selection from autocomplete
 */
async function selectMovie(movie) {
    selectedMovie = movie;
    elements.searchInput.value = movie.title;
    hideAutocomplete();
    updateClearButton();

    setLoading(true);
    hideError();

    try {
        const recommendations = await getRecommendations(movie.id);
        renderSelectedMovie(movie);
        renderRecommendations(recommendations, movie);
    } catch (error) {
        showError('Failed to get recommendations. Please try again.');
    } finally {
        setLoading(false);
    }
}

/**
 * Handle clicking on a movie card
 */
async function handleCardClick(movieId) {
    // Find the movie in recommendations
    const movie = allRecommendations.find(m => m.id === movieId);
    if (movie) {
        elements.searchInput.value = movie.title;
        await selectMovie(movie);
        window.scrollTo({ top: 0, behavior: 'smooth' });
    }
}

/**
 * Handle keyboard navigation in autocomplete
 */
function handleKeyDown(event) {
    const items = document.querySelectorAll('.autocomplete-item');
    if (items.length === 0) return;

    switch (event.key) {
        case 'ArrowDown':
            event.preventDefault();
            selectedIndex = Math.min(selectedIndex + 1, items.length - 1);
            updateSelectedItem(items);
            break;
        case 'ArrowUp':
            event.preventDefault();
            selectedIndex = Math.max(selectedIndex - 1, 0);
            updateSelectedItem(items);
            break;
        case 'Enter':
            event.preventDefault();
            if (selectedIndex >= 0 && items[selectedIndex]) {
                items[selectedIndex].click();
            }
            break;
        case 'Escape':
            hideAutocomplete();
            break;
    }
}

/**
 * Update selected item in autocomplete
 */
function updateSelectedItem(items) {
    items.forEach((item, index) => {
        item.classList.toggle('selected', index === selectedIndex);
    });

    if (items[selectedIndex]) {
        items[selectedIndex].scrollIntoView({ block: 'nearest' });
    }
}

/**
 * Handle clear button click
 */
function handleClear() {
    elements.searchInput.value = '';
    updateClearButton();
    hideAutocomplete();
    elements.selectedMovieSection.hidden = true;
    elements.recommendationsSection.hidden = true;
    hideError();
    elements.searchInput.focus();
}

/**
 * Handle carousel scroll buttons
 */
function handleCarouselScroll(event) {
    const btn = event.target.closest('.carousel-btn');
    if (!btn) return;

    const carousel = btn.parentElement.querySelector('.carousel');
    const scrollAmount = carousel.clientWidth * 0.8;
    const direction = btn.dataset.direction === 'left' ? -1 : 1;

    carousel.scrollBy({
        left: scrollAmount * direction,
        behavior: 'smooth'
    });
}

/**
 * Handle scroll for navbar background
 */
function handleScroll() {
    const navbar = document.querySelector('.navbar');
    navbar.classList.toggle('scrolled', window.scrollY > 50);
}

// ============================================
// Event Listeners
// ============================================
document.addEventListener('DOMContentLoaded', () => {
    // Search input
    elements.searchInput.addEventListener('input', handleSearchInput);
    elements.searchInput.addEventListener('keydown', handleKeyDown);
    elements.searchInput.addEventListener('focus', () => {
        if (elements.searchInput.value.length >= 2) {
            handleSearchInput({ target: elements.searchInput });
        }
    });

    // Clear button
    elements.clearBtn.addEventListener('click', handleClear);

    // Click outside to close dropdown
    document.addEventListener('click', (event) => {
        if (!event.target.closest('.search-container')) {
            hideAutocomplete();
        }
    });

    // Carousel buttons
    document.querySelectorAll('.carousel-container').forEach(container => {
        container.addEventListener('click', handleCarouselScroll);
    });

    // Navbar scroll effect
    window.addEventListener('scroll', handleScroll);

    // Focus search on load
    elements.searchInput.focus();
});

// Expose for inline onclick
window.handleCardClick = handleCardClick;
