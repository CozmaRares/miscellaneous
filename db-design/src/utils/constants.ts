export const victoryTypes = Object.freeze([
  "draw",
  "mate",
  "resign",
  "outoftime",
] as const);

export const userColors = Object.freeze(["white", "black", "draw"] as const);

export const HTTP_PAGE_SIZE = 10;
