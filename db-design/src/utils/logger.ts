import { logger as girokLogger } from "girok";

const logger = girokLogger({
  formatter: "text",
  outputs: ["stdout"],
});

export default logger;
