# Movie Recommender System

A full-stack movie recommendation system using **Knowledge Graphs** and **Hash Tables** implemented in C, with a Python Flask backend and responsive web frontend.

![Tech Stack](https://img.shields.io/badge/C-Data%20Structures-blue)
![Backend](https://img.shields.io/badge/Python-Flask-green)
![Frontend](https://img.shields.io/badge/HTML%2FCSS%2FJS-Frontend-orange)

---

## 📁 Project Structure

```
movie-recommender/
├── c_engine/
│   ├── movie.h           # Data structures (Hash Table, Knowledge Graph)
│   ├── recommender.c     # Main C program with BFS recommendations
│   └── movies.txt        # 100 movie records
├── backend/
│   └── app.py            # Flask API server
├── frontend/
│   ├── index.html        # UI structure
│   ├── style.css         # Modern styling
│   └── script.js         # API integration
└── README.md             # This file
```

---

## 🚀 Quick Start

### Prerequisites
- **GCC Compiler** (for C code)
- **Python 3.7+** with pip
- Modern web browser

### Step 1: Compile the C Engine

```bash
cd c_engine

# On Windows (MinGW/MSYS2)
gcc -o recommender.exe recommender.c -lm

# On Linux/macOS
gcc -o recommender recommender.c -lm
```

### Step 2: Test the C Program

```bash
# Get recommendations for movie ID 9 (Inception)
./recommender 9     # Linux/macOS
recommender.exe 9   # Windows
```

Expected output:
```
Recommendations:
10,The Matrix,Sci-Fi,8.7
12,The Empire Strikes Back,Sci-Fi,8.7
14,Interstellar,Sci-Fi,8.6
...
```

### Step 3: Install Python Dependencies

```bash
cd ../backend
pip install flask flask-cors
```

### Step 4: Run the Backend Server

```bash
python app.py
```

The server will start at `http://localhost:5000`

### Step 5: Open the Frontend

Open `frontend/index.html` in your web browser, or serve it with:

```bash
cd ../frontend
python -m http.server 8080
```

Then visit `http://localhost:8080`

---

## 🔧 API Endpoints

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/recommend?movie_id=<id>` | GET | Get movie recommendations |
| `/movies` | GET | Get all movies |
| `/movie/<id>` | GET | Get specific movie |
| `/health` | GET | Health check |

### Example API Call

```bash
curl "http://localhost:5000/recommend?movie_id=9"
```

Response:
```json
[
  {"id": 10, "title": "The Matrix", "genre": "Sci-Fi", "rating": 8.7},
  {"id": 12, "title": "The Empire Strikes Back", "genre": "Sci-Fi", "rating": 8.7},
  {"id": 14, "title": "Interstellar", "genre": "Sci-Fi", "rating": 8.6}
]
```

---

## 📊 Data Structures Explained

### Hash Table (Movie Storage)

```
┌─────────────────────────────────────────┐
│           HASH TABLE                    │
│   Size: 127 (prime for distribution)   │
├─────────────────────────────────────────┤
│ Index 0: NULL                           │
│ Index 1: Movie(1) → Movie(128) → NULL   │  ← Separate chaining
│ Index 2: NULL                           │
│ Index 3: Movie(3) → NULL                │
│ ...                                     │
└─────────────────────────────────────────┘

Hash Function: movie_id % TABLE_SIZE
Collision Handling: Separate Chaining with Linked Lists
```

**Why Hash Table?**
- O(1) average-case lookup by movie ID
- Efficient insertion without reordering
- Direct access for recommendation results

### Knowledge Graph (Semantic Relationships)

```
┌─────────────────────────────────────────────────────────────┐
│                   KNOWLEDGE GRAPH                           │
│                                                             │
│   [The Matrix] ──GENRE_SIMILAR──► [Inception]               │
│        │                              │                     │
│        │                              │                     │
│   RATING_SIMILAR              GENRE_SIMILAR                 │
│        │                              │                     │
│        ▼                              ▼                     │
│   [Interstellar] ◄──RATING_SIMILAR── [2001: A Space Odyssey]│
│                                                             │
│   Edge Types:                                               │
│   ├── GENRE_SIMILAR: Same genre                             │
│   └── RATING_SIMILAR: Rating diff ≤ 0.5                     │
└─────────────────────────────────────────────────────────────┘
```

**Graph Construction:**
1. Each movie becomes a node
2. Movies with same genre → connected by `GENRE_SIMILAR`
3. Movies with rating difference ≤ 0.5 → connected by `RATING_SIMILAR`

### BFS Traversal for Recommendations

```
Starting from Movie ID 9 (Inception):

Level 0: [Inception]
         ↓ (explore neighbors)
Level 1: [The Matrix, Interstellar, The Empire Strikes Back]
         ↓ (explore their neighbors)
Level 2: [2001: A Space Odyssey, Aliens, Back to the Future]
         ↓
         ... (up to MAX_RECOMMENDATIONS)
         
Output: Movies discovered via BFS traversal
```

---

## ⏱️ Time & Space Complexity

### Hash Table Operations
| Operation | Average | Worst Case |
|-----------|---------|------------|
| Insert    | O(1)    | O(n)       |
| Search    | O(1)    | O(n)       |
| Space     | O(n)    | O(n)       |

### Knowledge Graph Operations
| Operation | Complexity |
|-----------|------------|
| Build Graph | O(n²) |
| Add Relation | O(1) |
| BFS Traversal | O(V + E) |
| Space | O(V + E) |

Where n = number of movies, V = vertices, E = edges

---

## 🧠 Why Knowledge Graph vs Simple Graph?

| Feature | Simple Graph | Knowledge Graph |
|---------|--------------|-----------------|
| Edge Semantics | None (just connected) | Rich (relationship types) |
| Query Types | "What's connected?" | "Why is it connected?" |
| Recommendations | Generic neighbors | Context-aware suggestions |
| Extensibility | Limited | Add new relationship types easily |

**Knowledge Graph Advantages:**
1. **Semantic Richness**: Edges carry meaning (genre, rating, etc.)
2. **Explainable**: Can explain WHY movies are similar
3. **Flexible**: Easy to add new relationship types (director, actor, etc.)
4. **Industry Standard**: Used by Netflix, Amazon, Google

---

## 📋 Movies Dataset

The `movies.txt` file contains 100 movies with:
- **Genres**: Action, Drama, Comedy, Romance, Sci-Fi, Thriller, Horror, Animation
- **Ratings**: 8.1 to 9.3 (IMDB-like scale)
- **IDs**: 1 to 100 (sequential)

Sample entries:
```
1,The Shawshank Redemption,Drama,9.3
9,Inception,Sci-Fi,8.8
20,Spirited Away,Animation,8.6
```

---

## 🔄 System Flow

```
┌──────────────┐    HTTP GET     ┌──────────────┐    subprocess    ┌──────────────┐
│   Frontend   │ ──────────────► │    Flask     │ ──────────────► │   C Engine   │
│  (Browser)   │                 │   Backend    │                 │ (recommender)│
└──────────────┘                 └──────────────┘                 └──────────────┘
       │                                │                                │
       │ 1. User enters                 │ 2. Calls C                     │ 3. BFS on
       │    Movie ID                    │    executable                  │    Knowledge
       │                                │                                │    Graph
       │                                │                                │
       ▼                                ▼                                ▼
┌──────────────┐    JSON         ┌──────────────┐    CSV stdout    ┌──────────────┐
│   Display    │ ◄────────────── │  Parse &     │ ◄────────────── │ Recommended  │
│   Results    │                 │  Convert     │                 │    Movies    │
└──────────────┘                 └──────────────┘                 └──────────────┘
```

---

## 🛠️ Troubleshooting

### C Compilation Errors
```bash
# Missing math library
gcc -o recommender recommender.c -lm

# On Windows, ensure MinGW is in PATH
```

### Backend Not Finding Executable
- Ensure `recommender.exe` (Windows) or `recommender` (Linux) exists in `c_engine/`
- Check the path in `app.py` if running from a different directory

### CORS Errors
- Make sure Flask-CORS is installed: `pip install flask-cors`
- Backend must be running at `http://localhost:5000`

---

## 📚 Further Improvements

1. **Add more relationship types**: Director, Actor, Year similarities
2. **Weighted edges**: Prioritize stronger relationships
3. **User preferences**: Learn from user ratings
4. **Caching**: Cache graph construction for faster startup
5. **Database integration**: Replace file storage with SQLite

---

## 📄 License

This project is for educational purposes (Data Structures course project).

---

**Built with ❤️ using C, Python, and Modern Web Technologies**
