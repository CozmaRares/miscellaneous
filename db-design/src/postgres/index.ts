import { env } from "@/env";
import { Pool } from "pg";
import { z, type output, type ZodRawShape } from "zod";
import type { pgTable } from "./schema";
import logger from "@/utils/logger";

let pool: Pool | null = null;

function getPool() {
  if (pool === null) {
    pool = new Pool({
      user: env.POSTGRES_USER,
      host: env.POSTGRES_HOST,
      database: env.POSTGRES_DB,
      password: env.POSTGRES_PASSWORD,
      port: env.POSTGRES_PORT,
    });
  }
  return pool;
}

export type ZodOutput<T extends ZodRawShape> = { [k in keyof T]: output<T[k]> };

export async function pgQuery<Shape extends ZodRawShape>(
  table: pgTable<Shape>,
  query: string,
  params?: unknown[],
): Promise<ZodOutput<Shape>[]> {
  query = query
    .replaceAll("__table__", table.table)
    .split(/\n\r?/)
    .map(line => line.trim())
    .filter(line => line.length > 0)
    .join(" ");

  if (params == undefined) logger.info("Running query:", query);
  else logger.info("Running query:", query, "with", JSON.stringify(params));

  const r = await getPool().query(query, params);
  return z.array(table.zod).parse(r.rows);
}
