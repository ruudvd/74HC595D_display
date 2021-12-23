# 74HC595D_display
ESPhome component to control led board

### Example yaml
```yaml

external_components:
  - source:
      type: git
      url: https://github.com/ruudvd/74HC595Display.git
      ref: develop
    components: [ 74HC595Display ]

display:
  platform: 74HC595Display

```
