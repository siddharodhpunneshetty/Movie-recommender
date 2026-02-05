/**
 * recommender.c - Movie Recommender System using Hash Table and Knowledge Graph
 *
 * This program:
 * 1. Loads movies from movies.txt into a hash table
 * 2. Builds a knowledge graph with semantic relationships
 * 3. Uses BFS to find recommendations based on movie similarity
 *
 * Usage: ./recommender <movie_id>
 * Output: CSV format recommendations to stdout
 */

#include "movie.h"

// ============== GLOBAL DATA STRUCTURES ==============
HashTable movieTable;
KnowledgeGraph movieGraph;
Movie allMovies[MAX_MOVIES];
int totalMovies = 0;

/**
 * loadMoviesFromFile - Load all movies from movies.txt
 * @param filename: Path to the movies file
 * @return: Number of movies loaded, -1 on error
 *
 * File format: id,title,genre,rating
 */
int loadMoviesFromFile(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "Error: Cannot open file %s\n", filename);
    return -1;
  }

  char line[256];
  int count = 0;

  while (fgets(line, sizeof(line), file) != NULL && count < MAX_MOVIES) {
    // Remove newline
    line[strcspn(line, "\n")] = '\0';

    // Parse CSV: id,title,genre,rating
    Movie movie;
    char *token;

    // Parse ID
    token = strtok(line, ",");
    if (token == NULL)
      continue;
    movie.id = atoi(token);

    // Parse Title
    token = strtok(NULL, ",");
    if (token == NULL)
      continue;
    strncpy(movie.title, token, MAX_TITLE_LEN - 1);
    movie.title[MAX_TITLE_LEN - 1] = '\0';

    // Parse Genre
    token = strtok(NULL, ",");
    if (token == NULL)
      continue;
    strncpy(movie.genre, token, MAX_GENRE_LEN - 1);
    movie.genre[MAX_GENRE_LEN - 1] = '\0';

    // Parse Rating
    token = strtok(NULL, ",");
    if (token == NULL)
      continue;
    movie.rating = atof(token);

    // Insert into hash table
    if (insertMovie(&movieTable, movie)) {
      allMovies[count] = movie;
      addMovieToGraph(&movieGraph, movie.id);
      count++;
    }
  }

  fclose(file);
  totalMovies = count;
  return count;
}

/**
 * buildKnowledgeGraph - Automatically construct relationships between movies
 *
 * Creates edges based on:
 * 1. GENRE_SIMILAR: Movies with the same genre
 * 2. RATING_SIMILAR: Movies with rating difference <= 0.5
 *
 * Time Complexity: O(n^2) where n is number of movies
 */
void buildKnowledgeGraph() {
  for (int i = 0; i < totalMovies; i++) {
    for (int j = i + 1; j < totalMovies; j++) {
      Movie *m1 = &allMovies[i];
      Movie *m2 = &allMovies[j];

      // Check genre similarity (same genre = connected)
      if (strcmp(m1->genre, m2->genre) == 0) {
        addRelation(&movieGraph, m1->id, m2->id, REL_GENRE_SIMILAR);
      }

      // Check rating similarity (difference <= 0.5)
      float ratingDiff = fabs(m1->rating - m2->rating);
      if (ratingDiff <= RATING_THRESHOLD) {
        addRelation(&movieGraph, m1->id, m2->id, REL_RATING_SIMILAR);
      }
    }
  }
}

/**
 * compareByScore - Compare two MovieScore structs for sorting
 *
 * Primary key: total_score (descending - higher score first)
 * Secondary key: rating (descending - higher rating first for tie-breaking)
 *
 * @param a: Pointer to first MovieScore
 * @param b: Pointer to second MovieScore
 * @return: Negative if a > b, positive if a < b, 0 if equal
 */
int compareByScore(const void *a, const void *b) {
  const MovieScore *scoreA = (const MovieScore *)a;
  const MovieScore *scoreB = (const MovieScore *)b;

  // Primary sort: by total_score (descending)
  if (scoreB->total_score != scoreA->total_score) {
    return scoreB->total_score - scoreA->total_score;
  }

  // Secondary sort: by rating (descending) for tie-breaking
  if (scoreB->rating > scoreA->rating)
    return 1;
  if (scoreB->rating < scoreA->rating)
    return -1;
  return 0;
}

/**
 * recommendMoviesWeighted - Find movie recommendations using weighted scoring
 *
 * @param kg: Pointer to the Knowledge Graph
 * @param ht: Pointer to the Hash Table for movie lookups
 * @param base_movie_id: ID of the movie to find recommendations for
 * @param recommendations: Array to store recommended movie IDs
 * @param maxRecommendations: Maximum number of recommendations to return
 * @return: Number of recommendations found
 *
 * ALGORITHM:
 * 1. For each edge from base movie in the knowledge graph:
 *    - GENRE_SIMILAR relationship: +5 points
 *    - RATING_SIMILAR relationship: +3 points
 * 2. Accumulate scores if multiple relationships exist
 * 3. Sort candidates by total_score (desc), then rating (desc)
 * 4. Return top N movies
 *
 * Time Complexity: O(E + N log N) where E = edges, N = candidates
 * Space Complexity: O(N) for storing candidate scores
 */
int recommendMoviesWeighted(KnowledgeGraph *kg, HashTable *ht,
                            int base_movie_id, int *recommendations,
                            int maxRecommendations) {
  // Get the node index for the base movie
  int nodeIndex = kg->movie_id_to_index[base_movie_id];
  if (nodeIndex == -1) {
    return 0; // Movie not found in graph
  }

  // Array to store scores for candidate movies
  // Index = movie_id, value = accumulated score (-1 = not a candidate)
  int scoreMap[MAX_MOVIES];
  for (int i = 0; i < MAX_MOVIES; i++) {
    scoreMap[i] = -1; // Initialize: not a candidate
  }

  // Get the source node and traverse its edges
  GraphNode *sourceNode = &kg->nodes[nodeIndex];
  GraphEdge *edge = sourceNode->edges;

  int candidateCount = 0;

  // PASS 1: Traverse all edges and accumulate scores
  while (edge != NULL) {
    int neighborId = edge->neighbor_id;

    // Skip the base movie itself
    if (neighborId != base_movie_id) {
      // Initialize score if this is a new candidate
      if (scoreMap[neighborId] == -1) {
        scoreMap[neighborId] = 0;
        candidateCount++;
      }

      // Add weighted score based on relationship type
      if (strcmp(edge->relation_type, REL_GENRE_SIMILAR) == 0) {
        scoreMap[neighborId] += WEIGHT_GENRE_SIMILAR;
      } else if (strcmp(edge->relation_type, REL_RATING_SIMILAR) == 0) {
        scoreMap[neighborId] += WEIGHT_RATING_SIMILAR;
      }
    }

    edge = edge->next;
  }

  // If no candidates found, return 0
  if (candidateCount == 0) {
    return 0;
  }

  // PASS 2: Build array of MovieScore structs for sorting
  MovieScore *candidates =
      (MovieScore *)malloc(candidateCount * sizeof(MovieScore));
  if (candidates == NULL) {
    fprintf(stderr, "Error: Memory allocation failed for candidates\n");
    return 0;
  }

  int idx = 0;
  for (int i = 0; i < MAX_MOVIES && idx < candidateCount; i++) {
    if (scoreMap[i] >= 0) {
      candidates[idx].movie_id = i;
      candidates[idx].total_score = scoreMap[i];

      // Get rating from hash table for tie-breaking
      Movie *movie = searchMovie(ht, i);
      candidates[idx].rating = (movie != NULL) ? movie->rating : 0.0f;

      idx++;
    }
  }

  // Sort candidates by score (desc), then rating (desc)
  qsort(candidates, candidateCount, sizeof(MovieScore), compareByScore);

  // Select top N recommendations
  int resultCount = (candidateCount < maxRecommendations) ? candidateCount
                                                          : maxRecommendations;

  for (int i = 0; i < resultCount; i++) {
    recommendations[i] = candidates[i].movie_id;
  }

  // Clean up
  free(candidates);

  return resultCount;
}

/**
 * printRecommendations - Output recommendations in CSV format
 * @param recommendations: Array of movie IDs
 * @param count: Number of recommendations
 */
void printRecommendations(int *recommendations, int count) {
  printf("Recommendations:\n");

  for (int i = 0; i < count; i++) {
    Movie *movie = searchMovie(&movieTable, recommendations[i]);
    if (movie != NULL) {
      printf("%d,%s,%s,%.1f\n", movie->id, movie->title, movie->genre,
             movie->rating);
    }
  }
}

/**
 * getScriptDirectory - Get directory of the executable
 * @param argv0: argv[0] from main
 * @param dirPath: Buffer to store directory path
 * @param size: Size of buffer
 */
void getScriptDirectory(const char *argv0, char *dirPath, size_t size) {
  strncpy(dirPath, argv0, size - 1);
  dirPath[size - 1] = '\0';

  // Find last path separator
  char *lastSlash = strrchr(dirPath, '/');
  char *lastBackslash = strrchr(dirPath, '\\');
  char *lastSep = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

  if (lastSep != NULL) {
    *(lastSep + 1) = '\0';
  } else {
    strcpy(dirPath, "./");
  }
}

/**
 * main - Entry point for the movie recommender
 *
 * Usage: ./recommender <movie_id>
 */
int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <movie_id>\n", argv[0]);
    fprintf(stderr, "Example: %s 25\n", argv[0]);
    return 1;
  }

  int movieId = atoi(argv[1]);
  if (movieId <= 0) {
    fprintf(stderr, "Error: Invalid movie ID. Must be a positive integer.\n");
    return 1;
  }

  // Initialize data structures
  initHashTable(&movieTable);
  initKnowledgeGraph(&movieGraph);

  // Construct path to movies.txt (same directory as executable)
  char dirPath[256];
  getScriptDirectory(argv[0], dirPath, sizeof(dirPath));

  char moviesPath[512];
  snprintf(moviesPath, sizeof(moviesPath), "%smovies.txt", dirPath);

  // Load movies from file
  int loaded = loadMoviesFromFile(moviesPath);
  if (loaded < 0) {
    // Try current directory as fallback
    loaded = loadMoviesFromFile("movies.txt");
    if (loaded < 0) {
      fprintf(stderr, "Error: Could not load movies from %s or movies.txt\n",
              moviesPath);
      return 1;
    }
  }

  // Verify the requested movie exists
  Movie *requestedMovie = searchMovie(&movieTable, movieId);
  if (requestedMovie == NULL) {
    fprintf(stderr, "Error: Movie with ID %d not found.\n", movieId);
    fprintf(stderr, "Available movie IDs: 1 to %d\n", totalMovies);
    freeHashTable(&movieTable);
    freeKnowledgeGraph(&movieGraph);
    return 1;
  }

  // Build the knowledge graph with semantic relationships
  buildKnowledgeGraph();

  // Get recommendations using weighted scoring algorithm
  int recommendations[MAX_RECOMMENDATIONS];
  int count = recommendMoviesWeighted(&movieGraph, &movieTable, movieId,
                                      recommendations, MAX_RECOMMENDATIONS);

  if (count == 0) {
    printf("No recommendations found for movie ID %d\n", movieId);
  } else {
    printRecommendations(recommendations, count);
  }

  // Cleanup
  freeHashTable(&movieTable);
  freeKnowledgeGraph(&movieGraph);

  return 0;
}
