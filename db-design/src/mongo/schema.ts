import { userColors, victoryTypes } from "@/utils/constants";
import mongoose from "mongoose";

type FieldType = keyof typeof mongoose.Schema.Types;

function pk(type: FieldType, extra?: any) {
  return {
    type: mongoose.Schema.Types[type],
    required: true,
    unique: true,
    ...extra,
  };
}

function fk(type: FieldType, ref: string, extra?: any) {
  return {
    type: mongoose.Schema.Types[type],
    ref,
    required: true,
    ...extra,
  };
}

function req(type: FieldType, extra?: any) {
  return {
    type: mongoose.Schema.Types[type],
    required: true,
    ...extra,
  };
}

const userSchema = new mongoose.Schema(
  {
    id: pk("String"),
    created_at: req("Date"),
  },
  { collection: "user" },
);
export const mongoUser = mongoose.model("User", userSchema);

const gameOpeningSchema = new mongoose.Schema(
  {
    eco: pk("String"),
    name: req("String"),
    created_at: req("Date"),
  },
  { collection: "game_opening" },
);
export const mongoGameOpening = mongoose.model(
  "GameOpening",
  gameOpeningSchema,
);

const timeControlSchema = new mongoose.Schema(
  {
    id: pk("Number"),
    min: req("Number"),
    sec: req("Number"),
    created_at: req("Date"),
  },
  { collection: "time_control" },
);
export const mongoTimeControl = mongoose.model(
  "TimeControl",
  timeControlSchema,
);

const gameSchema = new mongoose.Schema(
  {
    id: pk("String"),
    is_rated: req("Boolean"),
    no_turns: req("Number"),
    victory_type: req("String", { enum: victoryTypes }),
    moves: req("String"),
    time_ctrl_id: fk("Number", "TimeControl"),
    opening_eco: fk("String", "GameOpening"),
    opening_ply: req("Number"),
    created_at: req("Date"),
  },
  { collection: "game" },
);
export const mongoGame = mongoose.model("Game", gameSchema);

const gameToUserSchema = new mongoose.Schema(
  {
    game_id: fk("String", "Game"),
    user_id: fk("String", "User"),
    user_rating: req("Number"),
    user_color: req("String", { enum: userColors }),
    user_won: req("Boolean"),
    created_at: req("Date"),
  },
  { collection: "game_to_user" },
);
export const mongoGameToUser = mongoose.model("GameToUser", gameToUserSchema);

/*
db.user.deleteMany({});
db.game_opening.deleteMany({});
db.time_control.deleteMany({});
db.game.deleteMany({});
db.game_to_user.deleteMany({});
 */
