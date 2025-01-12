import { userColors, victoryTypes } from "@/utils/constants";
import { z, ZodObject, type ZodRawShape } from "zod";

export type pgTable<Shape extends ZodRawShape> = {
  table: string;
  zod: ZodObject<Shape>;
};
function table<Shape extends ZodRawShape>(
  table: string,
  zod: ZodObject<Shape>,
): pgTable<Shape> {
  return {
    zod,
    table: `processed.${table}`,
  };
}

const dateSchema = z.date().default(new Date());

export const pgUser = table(
  "user",
  z
    .object({
      id: z.string(),
      created_at: dateSchema,
    })
    .strict(),
);

export const pgGameOpening = table(
  "game_opening",
  z
    .object({
      eco: z.string(),
      name: z.string(),
      created_at: dateSchema,
    })
    .strict(),
);

export const pgTimeControl = table(
  "time_control",
  z
    .object({
      id: z.number().int(),
      min: z.number().int(),
      sec: z.number().int(),
      created_at: dateSchema,
    })
    .strict(),
);

export const pgGame = table(
  "game",
  z
    .object({
      id: z.string(),
      is_rated: z.boolean(),
      no_turns: z.number().int(),
      victory_type: z.enum(victoryTypes),
      moves: z.string(),
      time_ctrl_id: z.number().int(),
      opening_eco: z.string(),
      opening_ply: z.number().int(),
      created_at: dateSchema,
    })
    .strict(),
);

export const pgGameToUser = table(
  "game_to_user",
  z
    .object({
      game_id: z.string(),
      user_id: z.string(),
      user_rating: z.number().int(),
      user_color: z.enum(userColors),
      user_won: z.boolean(),
      created_at: dateSchema,
    })
    .strict(),
);
