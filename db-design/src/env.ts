import { createEnv } from "@t3-oss/env-core";
import { z } from "zod";

export const env = createEnv({
  server: {
    POSTGRES_USER: z.string(),
    POSTGRES_PASSWORD: z.string(),
    POSTGRES_HOST: z.string(),
    POSTGRES_PORT: z.string().transform((p, ctx) => {
      const parsed = parseInt(p);
      if (isNaN(parsed)) {
        ctx.addIssue({
          code: z.ZodIssueCode.custom,
          message: "Invalid integer value",
        });
        return z.NEVER;
      }
      return parsed;
    }),
    POSTGRES_DB: z.string(),
    MONGO_URI: z.string(),
    PORT: z.string().transform((p, ctx) => {
      const parsed = parseInt(p);
      if (isNaN(parsed)) {
        ctx.addIssue({
          code: z.ZodIssueCode.custom,
          message: "Invalid integer value",
        });
        return z.NEVER;
      }
      return parsed;
    }),
    NODE_ENV: z
      .enum(["development", "test", "production"])
      .default("development"),
  },

  clientPrefix: "PUBLIC_",
  client: {},
  runtimeEnv: process.env,
  emptyStringAsUndefined: true,
});
