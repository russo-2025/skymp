import * as sp from "skyrimPlatform";

export interface AnimDebugSettings {
  isActive?: boolean;
  animListCount?: number;
  animListStartPos?: { x: number, y: number },
  animListYPosDelta?: number;
  animKeys?: { [index: number]: string };
}

type AnimListItem = {
  name: string,
  textId: number,
  color: number[]
}

const playerId = 0x14;
const animationSucceededTextColor = [255, 255, 255, 1];
const animationNotSucceededTextColor = [255, 0, 0, 1];

class AnimQueueCollection {
  public static readonly Name = "AnimQueueCollection";

  constructor(settings: AnimDebugSettings) {
    const animListCount = settings.animListCount ?? 5;
    const animListStartPos = settings.animListStartPos ?? { x: 650, y: 600 };
    const animListYPosDelta = settings.animListYPosDelta ?? 32;
    let y = animListStartPos.y;

    this.list = new Array<AnimListItem>(animListCount);
    for (let idx = 0; idx < animListCount; ++idx) {
      this.list[idx] = { name: "", textId: sp.createText(animListStartPos.x, y, "", animationSucceededTextColor), color: animationSucceededTextColor };
      y += animListYPosDelta;
    }
  }

  private readonly list: Array<AnimListItem>;

  public clearSPText(): void {
    if (this.list.length === 0) return;
    this.list.forEach(item => sp.destroyText(item.textId));
  }

  public push(animName: string, color: number[]): void {
    let prevItem: AnimListItem | null = null;
    for (let idx = this.list.length - 1; idx >= 0; --idx) {
      const item = this.list[idx];
      prevItem = { ...item };

      item.name = animName;
      item.color = color;
      sp.setTextString(item.textId, item.name);
      sp.setTextColor(item.textId, item.color);

      animName = prevItem.name;
      color = prevItem.color;
    }
  }
}

if (sp.storage[AnimQueueCollection.Name] && (sp.storage[AnimQueueCollection.Name] as AnimQueueCollection).clearSPText) {
  (sp.storage[AnimQueueCollection.Name] as AnimQueueCollection)?.clearSPText();
}

export const init = (settings: AnimDebugSettings): void => {
  if (!settings || !settings.isActive) return;

  var queue = new AnimQueueCollection(settings);
  sp.storage[AnimQueueCollection.name] = queue;

  sp.hooks.sendAnimationEvent.add({
    enter: (ctx) => { },
    leave: (ctx) => {
      queue.push(ctx.animEventName, ctx.animationSucceeded ? animationSucceededTextColor : animationNotSucceededTextColor);
    }
  }, playerId, playerId);

  if (settings.animKeys) {
    sp.on("buttonEvent", (e) => {
      if (e.isUp && settings.animKeys![e.code]) {
        sp.Debug.sendAnimationEvent(sp.Game.getPlayer()!, settings.animKeys![e.code]);
      }
    });
  }
}
