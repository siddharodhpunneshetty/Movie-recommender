"""
app.py - Flask Backend for Movie Recommender System (Production-Ready)

PRODUCTION DEPLOYMENT NOTES:
- Configured for Linux deployment (Render/Railway)
- Serves frontend static files directly
- Uses relative paths for C executable
- Debug mode disabled for production

Endpoints:
    GET /                   - Serve frontend (index.html)
    GET /search?name=<query> - Search movies by name (fuzzy match)
    GET /movies             - Get all movies for autocomplete
    GET /movie/<id>         - Get specific movie details
    GET /recommend?movie_id=ID - Get movie recommendations
    GET /health             - Health check
"""

from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import subprocess
import os

# Initialize Flask app with static folder configuration
# Static files (index.html, style.css, script.js) are served from 'static' folder
app = Flask(__name__, static_folder='static', static_url_path='')
CORS(app)  # Enable CORS for cross-origin requests

# ==============================================================================
# PATH CONFIGURATION (Production-Ready)
# ==============================================================================

# Get directory containing this script
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# C Engine directory (relative path - works on both Windows and Linux)
C_ENGINE_DIR = os.path.join(SCRIPT_DIR, '..', 'c_engine')

# IMPORTANT: For Linux deployment, always use the Linux executable name
# Compile with: gcc -o recommender recommender.c -lm
RECOMMENDER_EXE = os.path.join(C_ENGINE_DIR, 'recommender')

# Movies data file path
MOVIES_FILE = os.path.join(C_ENGINE_DIR, 'movies.txt')

# ==============================================================================
# IN-MEMORY DATA STRUCTURES
# ==============================================================================

# Movie data storage
all_movies = []           # List of all movie objects
id_to_movie = {}          # Quick lookup by ID
name_to_movie = {}        # Quick lookup by lowercase name


def load_movies():
    """
    Load all movies from movies.txt and build lookup maps.
    Called once at server startup.
    """
    global all_movies, id_to_movie, name_to_movie
    
    movies = []
    try:
        with open(MOVIES_FILE, 'r', encoding='utf-8') as f:
            for line in f:
                line = line.strip()
                if line:
                    parts = line.split(',')
                    if len(parts) >= 4:
                        movie = {
                            'id': int(parts[0]),
                            'title': parts[1],
                            'genre': parts[2],
                            'rating': float(parts[3])
                        }
                        movies.append(movie)
                        id_to_movie[movie['id']] = movie
                        # Store lowercase title for case-insensitive search
                        name_to_movie[movie['title'].lower()] = movie
    except FileNotFoundError:
        # Log warning but don't crash - allows graceful startup
        pass
    
    all_movies = movies
    return movies


def parse_recommendations(output):
    """
    Parse C program output to list of movie dictionaries.
    Expected format: id,title,genre,rating (CSV per line)
    """
    recommendations = []
    lines = output.strip().split('\n')
    
    for line in lines:
        line = line.strip()
        # Skip header and error lines
        if line.startswith('Recommendations:') or not line:
            continue
        if line.startswith('Error:') or line.startswith('No recommendations'):
            continue
            
        parts = line.split(',')
        if len(parts) >= 4:
            try:
                movie = {
                    'id': int(parts[0]),
                    'title': parts[1],
                    'genre': parts[2],
                    'rating': float(parts[3])
                }
                recommendations.append(movie)
            except (ValueError, IndexError):
                continue
    
    return recommendations


# ==============================================================================
# FRONTEND ROUTE (Serve Static Files)
# ==============================================================================

@app.route('/')
def home():
    """
    Serve the frontend index.html from static folder.
    This allows the entire app to be served from a single Flask server.
    """
    return send_from_directory('static', 'index.html')


# ==============================================================================
# API ROUTES
# ==============================================================================

@app.route('/search', methods=['GET'])
def search_movies():
    """
    Search movies by name (case-insensitive, partial match).
    
    Query Parameters:
        name (str): The movie name to search for
    
    Returns:
        JSON array of matching movies (max 10 results)
    """
    query = request.args.get('name', '').strip().lower()
    
    if not query:
        return jsonify([])
    
    matches = []
    
    # First, check for exact match
    if query in name_to_movie:
        matches.append(name_to_movie[query])
    
    # Then find partial matches
    for movie in all_movies:
        title_lower = movie['title'].lower()
        if query in title_lower and movie not in matches:
            matches.append(movie)
            if len(matches) >= 10:
                break
    
    return jsonify(matches)


@app.route('/recommend', methods=['GET'])
def recommend():
    """
    Get movie recommendations based on a movie ID.
    Calls the compiled C executable using subprocess.
    
    Query Parameters:
        movie_id (int): The ID of the movie to get recommendations for
    
    Returns:
        JSON array of recommended movies
    """
    movie_id = request.args.get('movie_id', type=int)
    
    if movie_id is None:
        return jsonify({
            'error': 'Missing required parameter: movie_id',
            'usage': '/recommend?movie_id=<integer>'
        }), 400
    
    if movie_id <= 0:
        return jsonify({
            'error': 'Invalid movie_id. Must be a positive integer.'
        }), 400
    
    if not os.path.exists(RECOMMENDER_EXE):
        return jsonify({
            'error': 'Recommender engine not found. Please compile the C program.',
            'hint': 'Run: gcc -o recommender recommender.c -lm'
        }), 500
    
    try:
        # Execute C program with movie_id as argument
        # capture_output=True captures stdout and stderr
        # text=True returns strings instead of bytes
        result = subprocess.run(
            [RECOMMENDER_EXE, str(movie_id)],
            capture_output=True,
            text=True,
            timeout=10,
            cwd=C_ENGINE_DIR  # Run from c_engine directory for file access
        )
        
        if result.returncode != 0:
            error_msg = result.stderr.strip() if result.stderr else 'Unknown error'
            return jsonify({
                'error': f'Recommendation failed: {error_msg}'
            }), 404
        
        recommendations = parse_recommendations(result.stdout)
        return jsonify(recommendations)
    
    except subprocess.TimeoutExpired:
        return jsonify({'error': 'Recommendation timed out.'}), 504
    except Exception as e:
        return jsonify({'error': f'Internal server error: {str(e)}'}), 500


@app.route('/movies', methods=['GET'])
def get_all_movies():
    """Get all available movies for autocomplete."""
    return jsonify(all_movies)


@app.route('/movie/<int:movie_id>', methods=['GET'])
def get_movie(movie_id):
    """Get a specific movie by ID."""
    if movie_id in id_to_movie:
        return jsonify(id_to_movie[movie_id])
    return jsonify({'error': f'Movie with ID {movie_id} not found'}), 404


@app.route('/health', methods=['GET'])
def health_check():
    """
    Health check endpoint for monitoring.
    Returns status, loaded movies count, and engine availability.
    """
    return jsonify({
        'status': 'healthy',
        'movies_loaded': len(all_movies),
        'engine_ready': os.path.exists(RECOMMENDER_EXE)
    })


@app.route('/api', methods=['GET'])
def api_docs():
    """API documentation endpoint."""
    return jsonify({
        'name': 'Movie Recommender API',
        'version': '2.0-production',
        'endpoints': {
            'GET /': 'Serve frontend',
            'GET /search?name=<query>': 'Search movies by name',
            'GET /recommend?movie_id=<id>': 'Get movie recommendations',
            'GET /movies': 'Get all movies',
            'GET /movie/<id>': 'Get specific movie',
            'GET /health': 'Health check'
        }
    })


# ==============================================================================
# SERVER STARTUP
# ==============================================================================

if __name__ == '__main__':
    # Load movies at startup
    load_movies()
    
    # Production server configuration:
    # - host='0.0.0.0' allows external connections (required for cloud deployment)
    # - port=5000 is the default Flask port (can be overridden by PORT env variable)
    # - debug=False for production (security and performance)
    
    port = int(os.environ.get('PORT', 5000))  # Support PORT env variable (Render/Railway)
    app.run(host='0.0.0.0', port=port)
